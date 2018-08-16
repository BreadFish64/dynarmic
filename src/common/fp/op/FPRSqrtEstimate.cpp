/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <array>

#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_types.h"
#include "common/fp/fpcr.h"
#include "common/fp/fpsr.h"
#include "common/fp/info.h"
#include "common/fp/op/FPRSqrtEstimate.h"
#include "common/fp/process_exception.h"
#include "common/fp/process_nan.h"
#include "common/fp/unpacked.h"
#include "common/safe_ops.h"

namespace Dynarmic::FP {

/// Input is a u0.9 fixed point number. Only values in [0.25, 1.0) are valid.
/// Output is a u0.8 fixed point number, with an implied 1 prefixed.
/// i.e.: The output is a value in [1.0, 2.0).
static u8 RecipSqrtEstimate(u64 a) {
    using LUT = std::array<u8, 512>;

    static const LUT lut = [] {
        LUT result{};
        for (u64 i = 128; i < result.size(); i++) {
            u64 a = i;

            // Convert to u.10 (with 8 significant bits), force to odd
            if (a < 256) {
                // [0.25, 0.5)
                a = a * 2 + 1;
            } else {
                // [0.5, 1.0)
                a = (a | 1) * 2;
            }

            // Calculate largest b which for which b < 1.0 / sqrt(a).
            // Start from b = 1.0 (in u.9) since b cannot be smaller.
            u64 b = 512;
            // u.10 * u.9 * u.9 -> u.28
            while (a * (b + 1) * (b + 1) < (1u << 28)) {
                b++;
            }

            // Round to nearest u0.8 (with implied set integer bit).
            result[static_cast<LUT::size_type>(i)] = static_cast<u8>((b + 1) / 2);
        }
        return result;
    }();

    return lut[a & 0x1FF];
}

template<typename FPT>
FPT FPRSqrtEstimate(FPT op, FPCR fpcr, FPSR& fpsr) {
    auto [type, sign, value] = FPUnpack<FPT>(op, fpcr, fpsr);

    if (type == FPType::SNaN || type == FPType::QNaN) {
        return FPProcessNaN(type, op, fpcr, fpsr);
    }

    if (type == FPType::Zero) {
        FPProcessException(FPExc::DivideByZero, fpcr, fpsr);
        return FPInfo<FPT>::Infinity(sign);
    }

    if (sign) {
        FPProcessException(FPExc::InvalidOp, fpcr, fpsr);
        return FPInfo<FPT>::DefaultNaN();
    }

    if (type == FPType::Infinity) {
        return FPInfo<FPT>::Zero(false);
    }

    const int result_exponent = (-(value.exponent + 1)) >> 1;
    const bool was_exponent_odd = (value.exponent) % 2 == 0;

    const u64 scaled = Safe::LogicalShiftRight(value.mantissa, normalized_point_position - (was_exponent_odd ? 7 : 8));
    const u64 estimate = RecipSqrtEstimate(scaled);

    const FPT bits_exponent = static_cast<FPT>(result_exponent + FPInfo<FPT>::exponent_bias);
    const FPT bits_mantissa = static_cast<FPT>(estimate << (FPInfo<FPT>::explicit_mantissa_width - 8));
    return (bits_exponent << FPInfo<FPT>::explicit_mantissa_width) | (bits_mantissa & FPInfo<FPT>::mantissa_mask);
}

template u32 FPRSqrtEstimate<u32>(u32 op, FPCR fpcr, FPSR& fpsr);
template u64 FPRSqrtEstimate<u64>(u64 op, FPCR fpcr, FPSR& fpsr);

} // namespace Dynarmic::FP 
