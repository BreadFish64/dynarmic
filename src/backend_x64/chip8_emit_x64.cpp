#include <unordered_map>
#include <unordered_set>

#include <fmt/ostream.h>

#include "backend_x64/abi.h"
#include "backend_x64/block_of_code.h"
#include "backend_x64/chip8_emit_x64.h"
#include "backend_x64/chip8_jitstate.h"
#include "backend_x64/devirtualize.h"
#include "backend_x64/emit_x64.h"
#include "common/address_range.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_types.h"
#include "common/variant_util.h"
#include "frontend/Chip8/location_descriptor.h"
#include "frontend/Chip8/types.h"
#include "frontend/ir/basic_block.h"
#include "frontend/ir/microinstruction.h"
#include "frontend/ir/opcodes.h"

// TODO: Have ARM flags in host flags and not have them use up GPR registers unless necessary.
// TODO: Actually implement that proper instruction selector you've always wanted to sweetheart.

namespace Dynarmic::BackendX64 {

	using namespace Xbyak::util;

	static Xbyak::Address MJitStateReg(Chip8::Reg reg) {
		return dword[r15 + offsetof(Chip8JitState, Reg) + sizeof(u32) * static_cast<size_t>(reg)];
	}

	Chip8EmitContext::Chip8EmitContext(RegAlloc& reg_alloc, IR::Block& block)
		: EmitContext(reg_alloc, block) {}

	Chip8::LocationDescriptor Chip8EmitContext::Location() const {
		return Chip8::LocationDescriptor{ block.Location() };
	}

	Chip8EmitX64::Chip8EmitX64(BlockOfCode& code, Chip8::UserConfig config, Chip8::Jit* jit_interface)
		: EmitX64(code), config(config), jit_interface(jit_interface)
	{
		GenMemoryAccessors();
		code.PreludeComplete();
	}

	Chip8EmitX64::~Chip8EmitX64() = default;

	Chip8EmitX64::BlockDescriptor Chip8EmitX64::Emit(IR::Block& block) {
		code.align();
		const u8* const entrypoint = code.getCurr();

		// Start emitting.
		EmitCondPrelude(block);

		RegAlloc reg_alloc{ code, Chip8JitState::SpillCount, SpillToOpArg<Chip8JitState> };
		Chip8EmitContext ctx{ reg_alloc, block };

		for (auto iter = block.begin(); iter != block.end(); ++iter) {
			IR::Inst* inst = &*iter;

			// Call the relevant Emit* member function.
			switch (inst->GetOpcode()) {

#define OPCODE(name, type, ...)                 \
        case IR::Opcode::name:                  \
            Chip8EmitX64::Emit##name(ctx, inst);  \
            break;
#define Chip8OPC(name, type, ...)                    \
        case IR::Opcode::Chip8##name:                \
            Chip8EmitX64::EmitChip8##name(ctx, inst);  \
            break;
#define A64OPC(...)
#define Chip8OPC(...)
#include "frontend/ir/opcodes.inc"
#undef OPCODE
#undef Chip8OPC
#undef A64OPC
#undef Chip8OPC

			default:
				ASSERT_MSG(false, "Invalid opcode: {}", inst->GetOpcode());
				break;
			}

			reg_alloc.EndOfAllocScope();
		}

		reg_alloc.AssertNoMoreUses();

		EmitAddCycles(block.CycleCount());
		EmitX64::EmitTerminal(block.GetTerminal(), block.Location());
		code.int3();

		const Chip8::LocationDescriptor descriptor{ block.Location() };
		Patch(descriptor, entrypoint);

		const size_t size = static_cast<size_t>(code.getCurr() - entrypoint);
		const Chip8::LocationDescriptor end_location{ block.EndLocation() };
		const auto range = boost::icl::discrete_interval<u32>::closed(descriptor.PC(), end_location.PC() - 1);
		Chip8EmitX64::BlockDescriptor block_desc{ entrypoint, size };
		block_descriptors.emplace(descriptor.UniqueHash(), block_desc);
		block_ranges.AddRange(range, descriptor);

		return block_desc;
	}

	void Chip8EmitX64::ClearCache() {
		EmitX64::ClearCache();
		block_ranges.ClearCache();
	}

	void Chip8EmitX64::InvalidateCacheRanges(const boost::icl::interval_set<u32>& ranges) {
		InvalidateBasicBlocks(block_ranges.InvalidateRanges(ranges));
	}

	void Chip8EmitX64::GenMemoryAccessors() {
		code.align();
		read_memory_8 = code.getCurr<const void*>();
		ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::MemoryRead8).EmitCall(code);
		ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		code.ret();

		code.align();
		read_memory_16 = code.getCurr<const void*>();
		ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::MemoryRead16).EmitCall(code);
		ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		code.ret();

		code.align();
		read_memory_32 = code.getCurr<const void*>();
		ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::MemoryRead32).EmitCall(code);
		ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		code.ret();

		code.align();
		read_memory_64 = code.getCurr<const void*>();
		ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::MemoryRead64).EmitCall(code);
		ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		code.ret();

		code.align();
		write_memory_8 = code.getCurr<const void*>();
		ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::MemoryWrite8).EmitCall(code);
		ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		code.ret();

		code.align();
		write_memory_16 = code.getCurr<const void*>();
		ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::MemoryWrite16).EmitCall(code);
		ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		code.ret();

		code.align();
		write_memory_32 = code.getCurr<const void*>();
		ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::MemoryWrite32).EmitCall(code);
		ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		code.ret();

		code.align();
		write_memory_64 = code.getCurr<const void*>();
		ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::MemoryWrite64).EmitCall(code);
		ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, ABI_RETURN);
		code.ret();
	}

	void Chip8EmitX64::EmitChip8GetRegister(Chip8EmitContext& ctx, IR::Inst* inst) {
		Chip8::Reg reg = inst->GetArg(0).GetChip8RegRef();

		Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
		code.mov(result, MJitStateReg(reg));
		ctx.reg_alloc.DefineValue(inst, result);
	}

	void Chip8EmitX64::EmitChip8SetRegister(Chip8EmitContext& ctx, IR::Inst* inst) {
		auto args = ctx.reg_alloc.GetArgumentInfo(inst);
		Chip8::Reg reg = inst->GetArg(0).GetChip8RegRef();
		if (args[1].IsImmediate()) {
			code.mov(MJitStateReg(reg), args[1].GetImmediateU32());
		}
		else if (args[1].IsInXmm()) {
			Xbyak::Xmm to_store = ctx.reg_alloc.UseXmm(args[1]);
			code.movd(MJitStateReg(reg), to_store);
		}
		else {
			Xbyak::Reg32 to_store = ctx.reg_alloc.UseGpr(args[1]).cvt32();
			code.mov(MJitStateReg(reg), to_store);
		}
	}

	void Chip8EmitX64::EmitChip8WritePC(Chip8EmitContext& ctx, IR::Inst* inst) {
		auto args = ctx.reg_alloc.GetArgumentInfo(inst);
		auto& arg = args[0];

		if (arg.IsImmediate()) {
			u32 new_pc = arg.GetImmediateU32();
			code.mov(MJitStateReg(Chip8::Reg::PC), new_pc);
		}
		else {
				Xbyak::Reg32 new_pc = ctx.reg_alloc.UseScratchGpr(arg).cvt32();
				code.mov(MJitStateReg(Chip8::Reg::PC), new_pc);
		}
	}

	void Chip8EmitX64::EmitChip8CallSupervisor(Chip8EmitContext& ctx, IR::Inst* inst) {
		ctx.reg_alloc.HostCall(nullptr);

		code.SwitchMxcsrOnExit();
		code.mov(code.ABI_PARAM2, qword[r15 + offsetof(Chip8JitState, cycles_to_run)]);
		code.sub(code.ABI_PARAM2, qword[r15 + offsetof(Chip8JitState, cycles_remaining)]);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::AddTicks).EmitCall(code);
		ctx.reg_alloc.EndOfAllocScope();
		auto args = ctx.reg_alloc.GetArgumentInfo(inst);
		ctx.reg_alloc.HostCall(nullptr, {}, args[0]);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::CallSVC).EmitCall(code);
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::GetTicksRemaining).EmitCall(code);
		code.mov(qword[r15 + offsetof(Chip8JitState, cycles_to_run)], code.ABI_RETURN);
		code.mov(qword[r15 + offsetof(Chip8JitState, cycles_remaining)], code.ABI_RETURN);
		code.SwitchMxcsrOnEntry();
	}

	template <typename T, T(Chip8::UserCallbacks::*raw_fn)(Chip8::VAddr)>
	static void ReadMemory(BlockOfCode& code, RegAlloc& reg_alloc, IR::Inst* inst, const Chip8::UserConfig& config, const CodePtr wrapped_fn) {
		constexpr size_t bit_size = Common::BitSize<T>();
		auto args = reg_alloc.GetArgumentInfo(inst);

		if (!config.page_table) {
			reg_alloc.HostCall(inst, {}, args[0]);
			DEVIRT(config.callbacks, raw_fn).EmitCall(code);
			return;
		}

		reg_alloc.UseScratch(args[0], ABI_PARAM2);

		Xbyak::Reg64 result = reg_alloc.ScratchGpr({ ABI_RETURN });
		Xbyak::Reg32 vaddr = code.ABI_PARAM2.cvt32();
		Xbyak::Reg64 page_index = reg_alloc.ScratchGpr();
		Xbyak::Reg64 page_offset = reg_alloc.ScratchGpr();

		Xbyak::Label abort, end;

		code.mov(result, reinterpret_cast<u64>(config.page_table));
		code.mov(page_index.cvt32(), vaddr);
		code.shr(page_index.cvt32(), 12);
		code.mov(result, qword[result + page_index * 8]);
		code.test(result, result);
		code.jz(abort);
		code.mov(page_offset.cvt32(), vaddr);
		code.and_(page_offset.cvt32(), 4095);
		switch (bit_size) {
		case 8:
			code.movzx(result, code.byte[result + page_offset]);
			break;
		case 16:
			code.movzx(result, word[result + page_offset]);
			break;
		case 32:
			code.mov(result.cvt32(), dword[result + page_offset]);
			break;
		case 64:
			code.mov(result.cvt64(), qword[result + page_offset]);
			break;
		default:
			ASSERT_MSG(false, "Invalid bit_size");
			break;
		}
		code.jmp(end);
		code.L(abort);
		code.call(wrapped_fn);
		code.L(end);

		reg_alloc.DefineValue(inst, result);
	}

	template <typename T, void (Chip8::UserCallbacks::*raw_fn)(Chip8::VAddr, T)>
	static void WriteMemory(BlockOfCode& code, RegAlloc& reg_alloc, IR::Inst* inst, const Chip8::UserConfig& config, const CodePtr wrapped_fn) {
		constexpr size_t bit_size = Common::BitSize<T>();
		auto args = reg_alloc.GetArgumentInfo(inst);

		if (!config.page_table) {
			reg_alloc.HostCall(nullptr, {}, args[0], args[1]);
			DEVIRT(config.callbacks, raw_fn).EmitCall(code);
			return;
		}

		reg_alloc.ScratchGpr({ ABI_RETURN });
		reg_alloc.UseScratch(args[0], ABI_PARAM2);
		reg_alloc.UseScratch(args[1], ABI_PARAM3);

		Xbyak::Reg32 vaddr = code.ABI_PARAM2.cvt32();
		Xbyak::Reg64 value = code.ABI_PARAM3;
		Xbyak::Reg64 page_index = reg_alloc.ScratchGpr();
		Xbyak::Reg64 page_offset = reg_alloc.ScratchGpr();

		Xbyak::Label abort, end;

		code.mov(rax, reinterpret_cast<u64>(config.page_table));
		code.mov(page_index.cvt32(), vaddr);
		code.shr(page_index.cvt32(), 12);
		code.mov(rax, qword[rax + page_index * 8]);
		code.test(rax, rax);
		code.jz(abort);
		code.mov(page_offset.cvt32(), vaddr);
		code.and_(page_offset.cvt32(), 4095);
		switch (bit_size) {
		case 8:
			code.mov(code.byte[rax + page_offset], value.cvt8());
			break;
		case 16:
			code.mov(word[rax + page_offset], value.cvt16());
			break;
		case 32:
			code.mov(dword[rax + page_offset], value.cvt32());
			break;
		case 64:
			code.mov(qword[rax + page_offset], value.cvt64());
			break;
		default:
			ASSERT_MSG(false, "Invalid bit_size");
			break;
		}
		code.jmp(end);
		code.L(abort);
		code.call(wrapped_fn);
		code.L(end);
	}

	void Chip8EmitX64::EmitChip8ReadMemory8(Chip8EmitContext& ctx, IR::Inst* inst) {
		ReadMemory<u8, &Chip8::UserCallbacks::MemoryRead8>(code, ctx.reg_alloc, inst, config, read_memory_8);
	}

	void Chip8EmitX64::EmitChip8ReadMemory16(Chip8EmitContext& ctx, IR::Inst* inst) {
		ReadMemory<u16, &Chip8::UserCallbacks::MemoryRead16>(code, ctx.reg_alloc, inst, config, read_memory_16);
	}

	void Chip8EmitX64::EmitChip8WriteMemory8(Chip8EmitContext& ctx, IR::Inst* inst) {
		WriteMemory<u8, &Chip8::UserCallbacks::MemoryWrite8>(code, ctx.reg_alloc, inst, config, write_memory_8);
	}

	void Chip8EmitX64::EmitChip8WriteMemory16(Chip8EmitContext& ctx, IR::Inst* inst) {
		WriteMemory<u16, &Chip8::UserCallbacks::MemoryWrite16>(code, ctx.reg_alloc, inst, config, write_memory_16);
	}

	template <typename T, void (Chip8::UserCallbacks::*fn)(Chip8::VAddr, T)>
	static void ExclusiveWrite(BlockOfCode& code, RegAlloc& reg_alloc, IR::Inst* inst, const Chip8::UserConfig& config, bool prepend_high_word) {
		auto args = reg_alloc.GetArgumentInfo(inst);
		if (prepend_high_word) {
			reg_alloc.HostCall(nullptr, {}, args[0], args[1], args[2]);
		}
		else {
			reg_alloc.HostCall(nullptr, {}, args[0], args[1]);
		}
		Xbyak::Reg32 passed = reg_alloc.ScratchGpr().cvt32();
		Xbyak::Reg32 tmp = code.ABI_RETURN.cvt32(); // Use one of the unusued HostCall registers.

		Xbyak::Label end;

		code.mov(passed, u32(1));
		code.cmp(code.byte[r15 + offsetof(Chip8JitState, exclusive_state)], u8(0));
		code.je(end);
		code.mov(tmp, code.ABI_PARAM2);
		code.xor_(tmp, dword[r15 + offsetof(Chip8JitState, exclusive_address)]);
		code.test(tmp, Chip8JitState::RESERVATION_GRANULE_MASK);
		code.jne(end);
		code.mov(code.byte[r15 + offsetof(Chip8JitState, exclusive_state)], u8(0));
		if (prepend_high_word) {
			code.mov(code.ABI_PARAM3.cvt32(), code.ABI_PARAM3.cvt32()); // zero extend to 64-bits
			code.shl(code.ABI_PARAM4, 32);
			code.or_(code.ABI_PARAM3, code.ABI_PARAM4);
		}
		DEVIRT(config.callbacks, fn).EmitCall(code);
		code.xor_(passed, passed);
		code.L(end);

		reg_alloc.DefineValue(inst, passed);
	}

	void Chip8EmitX64::EmitTerminalImpl(IR::Term::Interpret terminal, IR::LocationDescriptor initial_location) {
		ASSERT_MSG(terminal.num_instructions == 1, "Unimplemented");

		code.mov(code.ABI_PARAM2.cvt32(), Chip8::LocationDescriptor{ terminal.next }.PC());
		code.mov(code.ABI_PARAM3.cvt32(), 1);
		code.mov(MJitStateReg(Chip8::Reg::PC), code.ABI_PARAM2.cvt32());
		code.SwitchMxcsrOnExit();
		DEVIRT(config.callbacks, &Chip8::UserCallbacks::InterpreterFallback).EmitCall(code);
		code.ReturnFromRunCode(true); // TODO: Check cycles
	}

	void Chip8EmitX64::EmitTerminalImpl(IR::Term::ReturnToDispatch, IR::LocationDescriptor) {
		code.ReturnFromRunCode();
	}

	void Chip8EmitX64::EmitTerminalImpl(IR::Term::LinkBlock terminal, IR::LocationDescriptor initial_location) {
		code.cmp(qword[r15 + offsetof(Chip8JitState, cycles_remaining)], 0);

		patch_information[terminal.next].jg.emplace_back(code.getCurr());
		if (auto next_bb = GetBasicBlock(terminal.next)) {
			EmitPatchJg(terminal.next, next_bb->entrypoint);
		}
		else {
			EmitPatchJg(terminal.next);
		}
		Xbyak::Label dest;
		code.jmp(dest, Xbyak::CodeGenerator::T_NEAR);

		code.SwitchToFarCode();
		code.align(16);
		code.L(dest);
		code.mov(MJitStateReg(Chip8::Reg::PC), Chip8::LocationDescriptor{ terminal.next }.PC());
		PushRSBHelper(rax, rbx, terminal.next);
		code.ForceReturnFromRunCode();
		code.SwitchToNearCode();
	}

	void Chip8EmitX64::EmitTerminalImpl(IR::Term::LinkBlockFast terminal, IR::LocationDescriptor initial_location) {
		patch_information[terminal.next].jmp.emplace_back(code.getCurr());
		if (auto next_bb = GetBasicBlock(terminal.next)) {
			EmitPatchJmp(terminal.next, next_bb->entrypoint);
		}
		else {
			EmitPatchJmp(terminal.next);
		}
	}

	void Chip8EmitX64::EmitTerminalImpl(IR::Term::PopRSBHint, IR::LocationDescriptor) {
		// This calculation has to match up with IREmitter::PushRSB
		code.mov(ecx, MJitStateReg(Chip8::Reg::PC));
		code.shl(rcx, 32);
		code.or_(rbx, rcx);

		code.mov(eax, dword[r15 + offsetof(Chip8JitState, rsb_ptr)]);
		code.sub(eax, 1);
		code.and_(eax, u32(Chip8JitState::RSBPtrMask));
		code.mov(dword[r15 + offsetof(Chip8JitState, rsb_ptr)], eax);
		code.cmp(rbx, qword[r15 + offsetof(Chip8JitState, rsb_location_descriptors) + rax * sizeof(u64)]);
		code.jne(code.GetReturnFromRunCodeAddress());
		code.mov(rax, qword[r15 + offsetof(Chip8JitState, rsb_codeptrs) + rax * sizeof(u64)]);
		code.jmp(rax);
	}

	void Chip8EmitX64::EmitTerminalImpl(IR::Term::If terminal, IR::LocationDescriptor initial_location) {
		Xbyak::Label pass = EmitCond(terminal.if_);
		EmitTerminal(terminal.else_, initial_location);
		code.L(pass);
		EmitTerminal(terminal.then_, initial_location);
	}

	void Chip8EmitX64::EmitTerminalImpl(IR::Term::CheckBit, IR::LocationDescriptor) {
		ASSERT_MSG(false, "Term::CheckBit should never be emitted by the Chip8 frontend");
	}

	void Chip8EmitX64::EmitTerminalImpl(IR::Term::CheckHalt terminal, IR::LocationDescriptor initial_location) {
		code.cmp(code.byte[r15 + offsetof(Chip8JitState, halt_requested)], u8(0));
		code.jne(code.GetForceReturnFromRunCodeAddress());
		EmitTerminal(terminal.else_, initial_location);
	}

	void Chip8EmitX64::EmitPatchJg(const IR::LocationDescriptor& target_desc, CodePtr target_code_ptr) {
		const CodePtr patch_location = code.getCurr();
		if (target_code_ptr) {
			code.jg(target_code_ptr);
		}
		else {
			code.mov(MJitStateReg(Chip8::Reg::PC), Chip8::LocationDescriptor{ target_desc }.PC());
			code.jg(code.GetReturnFromRunCodeAddress());
		}
		code.EnsurePatchLocationSize(patch_location, 14);
	}

	void Chip8EmitX64::EmitPatchJmp(const IR::LocationDescriptor& target_desc, CodePtr target_code_ptr) {
		const CodePtr patch_location = code.getCurr();
		if (target_code_ptr) {
			code.jmp(target_code_ptr);
		}
		else {
			code.mov(MJitStateReg(Chip8::Reg::PC), Chip8::LocationDescriptor{ target_desc }.PC());
			code.jmp(code.GetReturnFromRunCodeAddress());
		}
		code.EnsurePatchLocationSize(patch_location, 13);
	}

	void Chip8EmitX64::EmitPatchMovRcx(CodePtr target_code_ptr) {
		if (!target_code_ptr) {
			target_code_ptr = code.GetReturnFromRunCodeAddress();
		}
		const CodePtr patch_location = code.getCurr();
		code.mov(code.rcx, reinterpret_cast<u64>(target_code_ptr));
		code.EnsurePatchLocationSize(patch_location, 10);
	}

} // namespace Dynarmic::BackendX64
