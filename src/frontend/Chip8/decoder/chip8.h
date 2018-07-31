#pragma once

#include <algorithm>
#include <functional>
#include <vector>

#include <boost/optional.hpp>

#include "common/bit_util.h"
#include "common/common_types.h"
#include "frontend/decoder/decoder_detail.h"
#include "frontend/decoder/matcher.h"

namespace Dynarmic::Chip8 {

template <typename Visitor>
using Chip8Matcher = Decoder::Matcher<Visitor, u16>;

template <typename V>
std::vector<Chip8Matcher<V>> GetArmDecodeTable() {
    std::vector<Chip8Matcher<V>> table = {

#define INST(fn, name, bitstring)                                                                  \
    Decoder::detail::detail<Chip8Matcher<V>>::GetMatcher(fn, name, bitstring)
        // Branch instructions
        INST(&V::chip8_RET, "RET", "0000000011101110"),
        INST(&V::chip8_JP_NNN, "JP (3 nibbles)", "0001nnnnnnnnnnnn"),
        INST(&V::chip8_CALL_NNN, "CALL (3 nibbles)", "0010nnnnnnnnnnnn"),
        INST(&V::chip8_SE_XB, "SE (Vx, byte)", "0011xxxxkkkkkkkk"),
        INST(&V::chip8_SNE_XB, "SNE (Vx, byte)", "0100xxxxkkkkkkkk"),
        INST(&V::chip8_SE_XY, "SE (Vx, Vy)", "0101xxxxyyyy0000"),
        INST(&V::chip8_SNE_XY, "SNE (Vx, Vy)", "1001xxxxyyyy0000"),
        INST(&V::chip8_JP_ONNN, "JP (V0, nibble)", "0001nnnnnnnnnnnn"),
        INST(&V::chip8_SKP_X, "SKP (Vx)", "1110xxxx10011110"),
        INST(&V::chip8_SKNP_X, "SKNP (Vx)", "1110xxxx10100001"),

        // Data Processing instructions
        INST(&V::chip8_ADD_XB, "ADD (Vx, byte)", "0111xxxxkkkkkkkk"),
        INST(&V::chip8_OR_XY, "OR (Vx, Vy)", "1000xxxxyyyy0001"),
        INST(&V::chip8_AND_XY, "AND (Vx, Vy)", "1000xxxxyyyy0010"),
        INST(&V::chip8_XOR_XY, "XOR (Vx, Vy)", "1000xxxxyyyy0011"),
        INST(&V::chip8_ADD_XY, "ADD (Vx, Vy)", "1000xxxxyyyy0100"),
        INST(&V::chip8_SUB_XY, "SUB (Vx, Vy)", "1000xxxxyyyy0101"),
        INST(&V::chip8_SHR_X, "SHR (Vx)", "1000xxxx----0110"),
        INST(&V::chip8_SUBN_XY, "SUBN (Vx, Vy)", "1000xxxxyyyy0111"),
        INST(&V::chip8_SHL_X, "SHL (Vx)", "1000xxxx----1110"),
        INST(&V::chip8_AND_XY, "AND (Vx, Vy)", "1000xxxxyyyy0002"),
        INST(&V::chip8_ADD_IX, "ADD (I, Vx)", "1111xxxx00011110"),

        // Load/Store instructions
        INST(&V::chip8_LD_XB, "LD (Vx, byte)", "0110xxxxkkkkkkkk"),
        INST(&V::chip8_LD_XY, "LD (Vx, Vy)", "1000xxxxyyyy0000"),
        INST(&V::chip8_LD_INNN, "LD (I, 3 nibbles)", "1010nnnnnnnnnnnn"),
        INST(&V::chip8_LD_XDT, "LD (Vx, DT)", "1111xxxx00000111"),
        INST(&V::chip8_LD_XK, "LD (Vx, K)", "1111xxxx00001010"),
        INST(&V::chip8_LD_DTX, "LD (DT, Vx)", "1111xxxx00010101"),
        INST(&V::chip8_LD_STX, "LD (ST, Vx)", "1111xxxx00011000"),
        INST(&V::chip8_LD_FX, "LD (F, Vx)", "1111xxxx00101001"),

        // Load/Store Multiple instructions
        INST(&V::chip8_LD_BX, "LD (B, Vx)", "1111xxxx00110011"),
        INST(&V::chip8_LD_IX, "LD (I[], Vx)", "1111xxxx01010101"),
        INST(&V::chip8_LD_XI, "LD (Vx, I[])", "1111xxxx01100101"),

        // Miscellaneous instructions
        INST(&V::chip8_CLS, "CLS", "0000000011100000"),
        INST(&V::chip8_RNDXB, "RND (Vx, byte)", "1100xxxxkkkkkkkk"),
        INST(&V::chip8_DRWXYN, "DRW (Vx, Vy, nibble)", "1101xxxxyyyynnnn"),

#undef INST
    };

    // If a matcher has more bits in its mask it is more specific, so it should come first.
    std::stable_sort(table.begin(), table.end(), [](const auto& matcher1, const auto& matcher2) {
        return Common::BitCount(matcher1.GetMask()) > Common::BitCount(matcher2.GetMask());
    });

    return table;
}

template <typename V>
boost::optional<const Chip8Matcher<V>&> DecodeChip8(u32 instruction) {
    static const auto table = GetChip8DecodeTable<V>();

    const auto matches_instruction = [instruction](const auto& matcher) {
        return matcher.Matches(instruction);
    };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? boost::optional<const Chip8Matcher<V>&>(*iter) : boost::none;
}

} // namespace Dynarmic::Chip8
