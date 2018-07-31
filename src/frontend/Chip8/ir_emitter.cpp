#include "frontend/Chip8/ir_emitter.h"
#include "common/assert.h"
#include "frontend/ir/opcodes.h"

namespace Dynarmic::Chip8 {

	using Opcode = IR::Opcode;

	u32 IREmitter::PC() {
		return current_location.PC();
	}

	u32 IREmitter::AlignPC(size_t alignment) {
		u32 pc = PC();
		return static_cast<u32>(pc - pc % alignment);
	}

	IR::U32 IREmitter::GetRegister(Reg reg) {
		if (reg == Chip8::Reg::PC) {
			return Imm32(PC());
		}
		return Inst<IR::U32>(Opcode::Chip8GetRegister, IR::Value(reg));
	}

	void IREmitter::SetRegister(const Reg reg, const IR::U32& value) {
		ASSERT(reg != Chip8::Reg::PC);
		Inst(Opcode::Chip8SetRegister, IR::Value(reg), value);
	}

	void IREmitter::ALUWritePC(const IR::U32& value) {
		// This behaviour is ARM version-dependent.
		// The below implementation is for ARMv6k
		BranchWritePC(value);
	}

	void IREmitter::BranchWritePC(const IR::U32& value) {
		if (!current_location.TFlag()) {
			auto new_pc = And(value, Imm32(0xFFFFFFFC));
			Inst(Opcode::A32SetRegister, IR::Value(A32::Reg::PC), new_pc);
		}
		else {
			auto new_pc = And(value, Imm32(0xFFFFFFFE));
			Inst(Opcode::A32SetRegister, IR::Value(A32::Reg::PC), new_pc);
		}
	}

	void IREmitter::BXWritePC(const IR::U32& value) {
		Inst(Opcode::A32BXWritePC, value);
	}

	void IREmitter::LoadWritePC(const IR::U32& value) {
		// This behaviour is ARM version-dependent.
		// The below implementation is for ARMv6k
		BXWritePC(value);
	}

	void IREmitter::CallSupervisor(const IR::U32& value) {
		Inst(Opcode::A32CallSupervisor, value);
	}

	void IREmitter::ExceptionRaised(const Exception exception) {
		Inst(Opcode::A64ExceptionRaised, Imm32(PC()), Imm64(static_cast<u64>(exception)));
	}

} // namespace Dynarmic::Chip8
