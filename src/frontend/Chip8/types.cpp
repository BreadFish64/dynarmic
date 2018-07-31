#include <array>
#include <ostream>

#include "common/bit_util.h"
#include "frontend/Chip8/types.h"

namespace Dynarmic::Chip8 {

	const char* CondToString(Cond cond, bool explicit_al) {
		constexpr std::array<const char*, 15> cond_strs = {
			"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "al"
		};
		return (!explicit_al && cond == Cond::AL) ? "" : cond_strs.at(static_cast<size_t>(cond));
	}

	const char* RegToString(Reg reg) {
		constexpr std::array<const char*, 20> reg_strs = {
			"V0", "V1" "V2", "V3", "V4", "V5", "V6", "V7", "V8", "V9", "VA", "VB", "VC", "VD", "VE", "VF", "I", "DT", "ST", "SP", "PC"
		};
		return reg_strs.at(static_cast<size_t>(reg));
	}

	std::string RegListToString(RegList reg_list) {
		std::string ret = "";
		bool first_reg = true;
		for (size_t i = 0; i < 20; i++) {
			if (Common::Bit(i, reg_list)) {
				if (!first_reg)
					ret += ", ";
				ret += RegToString(static_cast<Reg>(i));
				first_reg = false;
			}
		}
		return ret;
	}

	std::ostream& operator<<(std::ostream& o, Reg reg) {
		o << RegToString(reg);
		return o;
	}

	std::ostream& operator<<(std::ostream& o, RegList reg_list) {
		o << RegListToString(reg_list);
		return o;
	}

} // namespace Dynarmic::Chip8
