/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

#include "common/bit_util.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::arm_CPS() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_MRS(Cond cond, Reg d) {
    if (d == Reg::PC)
        return UnpredictableInstruction();
    // MRS <Rd>, APSR
    if (ConditionPassed(cond)) {
        ir.SetRegister(d, ir.GetCpsr());
    }
    return true;
}

bool ArmTranslatorVisitor::arm_MSR_imm(Cond cond, int mask, int rotate, Imm8 imm8) {
    const bool write_nzcvq = Common::Bit<3>(mask);
    const bool write_g = Common::Bit<2>(mask);
    const bool write_e = Common::Bit<1>(mask);
    const u32 imm32 = ArmExpandImm(rotate, imm8);
    ASSERT_MSG(mask != 0, "Decode error");
    // MSR <spec_reg>, #<imm32>
    if (ConditionPassed(cond)) {
        if (write_nzcvq) {
            ir.SetCpsrNZCVQ(ir.Imm32(imm32 & 0xF8000000));
        }
        if (write_g) {
            ir.SetGEFlagsCompressed(ir.Imm32(imm32 & 0x000F0000));
        }
        if (write_e) {
            const bool E = (imm32 & 0x00000200) != 0;
            if (E != ir.current_location.EFlag()) {
                ir.SetTerm(IR::Term::LinkBlock{ir.current_location.AdvancePC(4).SetEFlag(E)});
                return false;
            }
        }
    }
    return true;
}

bool ArmTranslatorVisitor::arm_MSR_reg(Cond cond, int mask, Reg n) {
    const bool write_nzcvq = Common::Bit<3>(mask);
    const bool write_g = Common::Bit<2>(mask);
    const bool write_e = Common::Bit<1>(mask);
    if (mask == 0)
        return UnpredictableInstruction();
    if (n == Reg::PC)
        return UnpredictableInstruction();
    // MSR <spec_reg>, #<imm32>
    if (ConditionPassed(cond)) {
        const auto value = ir.GetRegister(n);
        if (!write_e) {
            if (write_nzcvq) {
                ir.SetCpsrNZCVQ(ir.And(value, ir.Imm32(0xF8000000)));
            }
            if (write_g) {
                ir.SetGEFlagsCompressed(ir.And(value, ir.Imm32(0x000F0000)));
            }
        } else {
            const u32 cpsr_mask = (write_nzcvq ? 0xF8000000 : 0) | (write_g ? 0x000F0000 : 0) | 0x00000200;
            const auto old_cpsr = ir.And(ir.GetCpsr(), ir.Imm32(~cpsr_mask));
            const auto new_cpsr = ir.And(value, ir.Imm32(cpsr_mask));
            ir.SetCpsr(ir.Or(old_cpsr, new_cpsr));
            ir.PushRSB(ir.current_location.AdvancePC(4));
            ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
            ir.SetTerm(IR::Term::CheckHalt{IR::Term::PopRSBHint{}});
            return false;
        }
    }
    return true;
}

bool ArmTranslatorVisitor::arm_RFE() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SETEND(bool E) {
    // SETEND {BE,LE}
    ir.SetTerm(IR::Term::LinkBlock{ir.current_location.AdvancePC(4).SetEFlag(E)});
    return false;
}

bool ArmTranslatorVisitor::arm_SRS() {
    return InterpretThisInstruction();
}

} // namespace Dynarmic::A32
