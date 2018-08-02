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

	IR::U8 IREmitter::GetRegister(Reg reg) {
		return Inst<IR::U8>(Opcode::Chip8GetRegister, IR::Value(reg));
	}

	void IREmitter::SetRegister(const Reg reg, const IR::U32& value) {
		ASSERT(reg != Chip8::Reg::PC);
		Inst(Opcode::Chip8SetRegister, IR::Value(reg), value);
	}

	void IREmitter::WritePC(const IR::U32& value) {
		Inst(Opcode::Chip8WritePC, value);
	}

	void IREmitter::CallSupervisor(const IR::U32& value) {
		Inst(Opcode::Chip8CallSupervisor, value);
	}

	IR::U8 IREmitter::ReadMemory8(const IR::U32& vaddr) {
		return Inst<IR::U8>(Opcode::Chip8ReadMemory8, vaddr);
	}

	IR::U16 IREmitter::ReadMemory16(const IR::U32& vaddr) {
		return Inst<IR::U16>(Opcode::Chip8ReadMemory16, vaddr);
	}

} // namespace Dynarmic::Chip8
