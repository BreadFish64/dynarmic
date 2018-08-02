#include <algorithm>

#include "common/assert.h"
#include "dynarmic/Chip8/config.h"
#include "frontend/Chip8/decoder/chip8.h"
#include "frontend/Chip8/location_descriptor.h"
#include "frontend/Chip8/translate/translate.h""
#include "frontend/Chip8/translate/translate_chip8/translate_chip8.h"
#include "frontend/Chip8/types.h"
#include "frontend/ir/basic_block.h"

namespace Dynarmic::Chip8 {

	static bool CondCanContinue(ConditionalState cond_state, const Chip8::IREmitter& ir) {
		ASSERT_MSG(cond_state != ConditionalState::Break, "Should never happen.");

		/*if (cond_state == ConditionalState::None)
			return true;

		// TODO: This is more conservative than necessary.
		return std::all_of(ir.block.begin(), ir.block.end(), [](const IR::Inst& inst) { return !inst.WritesToCPSR(); });*/
	}

	IR::Block TranslateChip8(LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code) {
		IR::Block block{ descriptor };
		Chip8TranslatorVisitor visitor{ block, descriptor };

		bool should_continue = true;
		while (should_continue && CondCanContinue(visitor.cond_state, visitor.ir)) {
			const u32 chip8_pc = visitor.ir.current_location.PC();
			const u32 chip8_instruction = memory_read_code(chip8_pc);

			if (auto decoder = DecodeChip8<Chip8TranslatorVisitor>(chip8_instruction)) {
				should_continue = decoder->call(visitor, chip8_instruction);
			} else {
				should_continue = visitor.UndefinedInstruction();
			}

			if (visitor.cond_state == ConditionalState::Break) {
				break;
			}

			visitor.ir.current_location = visitor.ir.current_location.AdvancePC(4);
			block.CycleCount()++;
		}

		if (visitor.cond_state == ConditionalState::Translating || visitor.cond_state == ConditionalState::Trailing) {
			if (should_continue) {
				visitor.ir.SetTerm(IR::Term::LinkBlockFast{ visitor.ir.current_location });
			}
		}

		ASSERT_MSG(block.HasTerminal(), "Terminal has not been set");

		block.SetEndLocation(visitor.ir.current_location);

		return block;
	}

	bool Chip8TranslatorVisitor::ConditionPassed(Cond cond) {
		ASSERT_MSG(cond_state != ConditionalState::Break,
			"This should never happen. We requested a break but that wasn't honored.");
		ASSERT_MSG(cond != Cond::NV, "NV conditional is obsolete");

		if (cond_state == ConditionalState::Translating) {
			if (ir.block.ConditionFailedLocation() != ir.current_location || cond == Cond::AL) {
				cond_state = ConditionalState::Trailing;
			}
			else {
				if (cond == ir.block.GetCondition()) {
					ir.block.SetConditionFailedLocation(ir.current_location.AdvancePC(4));
					ir.block.ConditionFailedCycleCount()++;
					return true;
				}

				// cond has changed, abort
				cond_state = ConditionalState::Break;
				ir.SetTerm(IR::Term::LinkBlockFast{ ir.current_location });
				return false;
			}
		}

		if (cond == Cond::AL) {
			// Everything is fine with the world
			return true;
		}

		// non-AL cond

		if (!ir.block.empty()) {
			// We've already emitted instructions. Quit for now, we'll make a new block here later.
			cond_state = ConditionalState::Break;
			ir.SetTerm(IR::Term::LinkBlockFast{ ir.current_location });
			return false;
		}

		// We've not emitted instructions yet.
		// We'll emit one instruction, and set the block-entry conditional appropriately.

		cond_state = ConditionalState::Translating;
		ir.block.SetCondition(cond);
		ir.block.SetConditionFailedLocation(ir.current_location.AdvancePC(4));
		ir.block.ConditionFailedCycleCount() = 1;
		return true;
	}

	bool Chip8TranslatorVisitor::InterpretThisInstruction() {
		ir.SetTerm(IR::Term::Interpret(ir.current_location));
		return false;
	}

	IR::ResultAndCarry<IR::U8> Chip8TranslatorVisitor::EmitShift(IR::U8 value, ShiftType type, IR::U1 carry_in) {
		switch (type) {
		case ShiftType::LSL:
			IR::ResultAndCarry<IR::U32> temp = ir.LogicalShiftLeft(IR::U32(value), ir.Imm8(1), carry_in);
			return { ir.LeastSignificantByte(temp.result) , temp.carry};
		case ShiftType::LSR:
			IR::ResultAndCarry<IR::U32> temp = ir.LogicalShiftRight(IR::U32(value), ir.Imm8(1), carry_in);
			return { ir.LeastSignificantByte(temp.result) , temp.carry };
		}
		ASSERT_MSG(false, "Unreachable");
		return {};
	}

} // namespace Dynarmic::Chip8
