#include "common/bit_util.h"

#include "translate_chip8.h"

namespace Dynarmic::Chip8 {
	
	bool Chip8TranslatorVisitor::chip8_RET() {
		ir.SetTerm(IR::Term::PopRSBHint{});
		return false;
	}

	bool Chip8TranslatorVisitor::chip8_JP_NNN(Imm12 addr) {
		auto new_location = ir.current_location.SetPC(addr);
		return false;
	}

	bool Chip8TranslatorVisitor::chip8_CALL_NNN(Imm12 addr) {
		ir.PushRSB(ir.current_location.AdvancePC(2));
		auto new_location = ir.current_location.SetPC(addr);
		ir.SetTerm(IR::Term::LinkBlock{ new_location });
		return false;
	}

	bool Chip8TranslatorVisitor::chip8_SE_XB(Reg x, Imm8 byte) {
		LocationDescriptor new_location = 0;
		if (ir.GetRegister(x).GetU8() == byte)
			new_location = ir.current_location.AdvancePC(4);
		else 
			new_location = ir.current_location.AdvancePC(2);
		ir.SetTerm(IR::Term::LinkBlock{ new_location });
		return false;
	}

	bool Chip8TranslatorVisitor::chip8_SNE_XB(Reg x, Imm8 byte) {
		LocationDescriptor new_location = 0;
		if (ir.GetRegister(x).GetU8() != byte)
			new_location = ir.current_location.AdvancePC(4);
		else
			new_location = ir.current_location.AdvancePC(2);
		ir.SetTerm(IR::Term::LinkBlock{ new_location });
		return false;
	}

	bool Chip8TranslatorVisitor::chip8_SE_XY(Reg x, Reg y) {
		LocationDescriptor new_location = 0;
		if (ir.GetRegister(x).GetU8() == ir.GetRegister(y).GetU8())
			new_location = ir.current_location.AdvancePC(4);
		else
			new_location = ir.current_location.AdvancePC(2);
		ir.SetTerm(IR::Term::LinkBlock{ new_location });
		return false;
	}

	bool Chip8TranslatorVisitor::chip8_SNE_XY(Reg x, Reg y) {
		LocationDescriptor new_location = 0;
		if (ir.GetRegister(x).GetU8() != ir.GetRegister(y).GetU8())
			new_location = ir.current_location.AdvancePC(4);
		else
			new_location = ir.current_location.AdvancePC(2);
		ir.SetTerm(IR::Term::LinkBlock{ new_location });
		return false;
	}

	bool Chip8TranslatorVisitor::chip8_JP_ONNN(Imm12 addr) {
		auto new_addr = ir.Or(ir.ZeroExtendByteToWord(ir.GetRegister(Reg::V0)), ir.Imm32(addr));
		auto new_location = ir.current_location.SetPC(addr);
		ir.SetTerm(IR::Term::LinkBlock{ new_location });
		return false;
	}

	bool Chip8TranslatorVisitor::chip8_SKP_X(Reg x) {
		LocationDescriptor new_location = 0;
		if (ir.GetRegister(x).GetU8() != byte)
			new_location = ir.current_location.AdvancePC(4);
		else
			new_location = ir.current_location.AdvancePC(2);
		ir.SetTerm(IR::Term::LinkBlock{ new_location });
		return false;
	}
}
