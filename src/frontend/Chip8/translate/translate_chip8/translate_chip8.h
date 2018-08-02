#pragma once

#include "common/bit_util.h"
#include "frontend/Chip8/ir_emitter.h"
#include "frontend/Chip8/location_descriptor.h"

namespace Dynarmic::Chip8 {

	enum class ConditionalState {
		/// We haven't met any conditional instructions yet.
		None,
		/// Current instruction is a conditional. This marks the end of this basic block.
		Break,
		/// This basic block is made up solely of conditional instructions.
		Translating,
		/// This basic block is made up of conditional instructions followed by unconditional instructions.
		Trailing,
	};

	struct Chip8TranslatorVisitor final {
		using instruction_return_type = bool;

		explicit Chip8TranslatorVisitor(IR::Block& block, LocationDescriptor descriptor) : ir(block, descriptor) {}

		Chip8::IREmitter ir;
		ConditionalState cond_state = ConditionalState::None;

		bool ConditionPassed(Cond cond);
		bool InterpretThisInstruction();
		bool UnpredictableInstruction();
		bool UndefinedInstruction();

		IR::ResultAndCarry<IR::U8> EmitShift(IR::U8 value, ShiftType type, IR::U1 b);

		// Branch instructions
		bool chip8_RET();
		bool chip8_JP_NNN(Imm12 addr);
		bool chip8_CALL_NNN(Imm12 addr);
		bool chip8_SE_XB(Reg x, Imm8 byte);
		bool chip8_SNE_XB(Reg x, Imm8 byte);
		bool chip8_SE_XY(Reg x, Reg y);
		bool chip8_SNE_XY(Reg x, Reg y);
		bool chip8_JP_ONNN(Imm12 addr);
		bool chip8_SKP_X(Reg x);
		bool chip8_SKNP_X(Reg x);

		// Data Processing instructions
		bool chip8_ADD_XB(Reg x, Imm8 byte);
		bool chip8_OR_XY(Reg x, Reg y);
		bool chip8_AND_XY(Reg x, Reg y);
		bool chip8_XOR_XY(Reg x, Reg y);
		bool chip8_ADD_XY(Reg x, Reg y);
		bool chip8_SUB_XY(Reg x, Reg y);
		bool chip8_SHR_XY(Reg x);
		bool chip8_SUBN_XY(Reg x, Reg y);
		bool chip8_SHL_X(Reg x);
		bool chip8_ADD_IX(Reg x);

		// Load/Store instructions
		bool chip8_LD_XB(Reg x, Imm8 byte);
		bool chip8_LD_XY(Reg x, Reg y);
		bool chip8_LD_INNN(Imm12 addr);
		bool chip8_LD_XDT(Reg x);
		bool chip8_LD_XK(Reg x);
		bool chip8_LD_DTX(Reg x);
		bool chip8_LD_STX(Reg x);
		bool chip8_LD_FX(Reg x);

		// Load/Store Multiple instructions
		bool chip8_LD_BX(Reg x);
		bool chip8_LD_IX(Reg x);
		bool chip8_LD_XI(Reg x);

		// Miscellaneous instructions
		bool chip8_CLS();
		bool chip8_RND_XB(Reg x, Imm8 byte);
		bool chip8_DRW_XYN(Reg x, Reg y, Imm4 nibble);
	};

} // namespace Dynarmic::Chip8
