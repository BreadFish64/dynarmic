/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <utility>
#include "common/assert.h"
#include "frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
std::pair<size_t, Vec> Combine(Imm<2> size, Imm<1> H, Imm<1> L, Imm<1> M, Imm<4> Vmlo) {
    if (size == 0b01) {
        return {concatenate(H, L, M).ZeroExtend(), Vmlo.ZeroExtend<Vec>()};
    }

    return {concatenate(H, L).ZeroExtend(), concatenate(M, Vmlo).ZeroExtend<Vec>()};
}

enum class ExtraBehavior {
    None,
    Accumulate,
    Subtract,
};

bool MultiplyByElement(TranslatorVisitor& v, bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd,
                       ExtraBehavior extra_behavior) {
    if (size != 0b01 && size != 0b10) {
        return v.UnallocatedEncoding();
    }

    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);
    const size_t idxdsize = H == 1 ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;
    
    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.ir.VectorBroadcast(esize, v.ir.VectorGetElement(esize, v.V(idxdsize, Vm), index));
    const IR::U128 operand3 = v.V(datasize, Vd);
    
    IR::U128 result = v.ir.VectorMultiply(esize, operand1, operand2);
    if (extra_behavior == ExtraBehavior::Accumulate) {
        result = v.ir.VectorAdd(esize, operand3, result);
    } else if (extra_behavior == ExtraBehavior::Subtract) {
        result = v.ir.VectorSub(esize, operand3, result);
    }

    v.V(datasize, Vd, result);
    return true;
}

bool FPMultiplyByElement(TranslatorVisitor& v, bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd,
                         ExtraBehavior extra_behavior) {
    if (sz && L == 1) {
        return v.UnallocatedEncoding();
    }
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t idxdsize = H == 1 ? 128 : 64;
    const size_t index = sz ? H.ZeroExtend() : concatenate(H, L).ZeroExtend();
    const Vec Vm = concatenate(M, Vmlo).ZeroExtend<Vec>();
    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::UAny element2 = v.ir.VectorGetElement(esize, v.V(idxdsize, Vm), index);
    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = Q ? v.ir.VectorBroadcast(esize, element2) : v.ir.VectorBroadcastLower(esize, element2);
    const IR::U128 operand3 = v.V(datasize, Vd);

    const IR::U128 result = [&]{
        switch (extra_behavior) {
        case ExtraBehavior::None:
            return v.ir.FPVectorMul(esize, operand1, operand2);
        case ExtraBehavior::Accumulate:
            return v.ir.FPVectorMulAdd(esize, operand3, operand1, operand2);
        case ExtraBehavior::Subtract:
            return v.ir.FPVectorMulAdd(esize, operand3, v.ir.FPVectorNeg(esize, operand1), operand2);
        }
        UNREACHABLE();
        return IR::U128{};
    }();
    v.V(datasize, Vd, result);
    return true;
}

} // Anonymous namespace

bool TranslatorVisitor::MLA_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate);
}

bool TranslatorVisitor::MLS_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract);
}

bool TranslatorVisitor::MUL_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::None);
}

bool TranslatorVisitor::FMLA_elt_4(bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElement(*this, Q, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate);
}

bool TranslatorVisitor::FMLS_elt_4(bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElement(*this, Q, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract);
}

bool TranslatorVisitor::FMUL_elt_4(bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElement(*this, Q, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::None);
}

} // namespace Dynarmic::A64
