#pragma once

#include <iosfwd>
#include <string>
#include <utility>

#include "common/assert.h"
#include "common/common_types.h"
#include "frontend/ir/cond.h"

namespace Dynarmic::Chip8 {

	using Cond = IR::Cond;

	enum class Reg {
		V0,
		V1,
		V2,
		V3,
		V4,
		V5,
		V6,
		V7,
		V8,
		V9,
		VA,
		VB,
		VC,
		VD,
		VE,
		VF,
		I,
		DT,
		ST,
		SP,
		PC,
		INVALID_REG = 99
	};

	using Imm4 = u8;
	using Imm8 = u8;
	using Imm12 = u16;
	using Imm16 = u16;
	using RegList = u32;

	enum class ShiftType {
		LSL,
		LSR
	};

	const char* CondToString(Cond cond, bool explicit_al = false);
	const char* RegToString(Reg reg);
	std::string RegListToString(RegList reg_list);

	std::ostream& operator<<(std::ostream& o, Reg reg);
	std::ostream& operator<<(std::ostream& o, RegList reg_list);

	inline size_t RegNumber(Reg reg) {
		ASSERT(reg != Reg::INVALID_REG);
		return static_cast<size_t>(reg);
	}

	inline Reg operator+(Reg reg, size_t number) {
		ASSERT(reg != Reg::INVALID_REG);

		size_t new_reg = static_cast<size_t>(reg) + number;
		ASSERT(new_reg <= 20);

		return static_cast<Reg>(new_reg);
	}

} // namespace Dynarmic::Chip8
