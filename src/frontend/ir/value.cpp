/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "frontend/ir/value.h"
#include "common/assert.h"
#include "frontend/ir/microinstruction.h"
#include "frontend/ir/opcodes.h"
#include "frontend/ir/type.h"

namespace Dynarmic::IR {

Value::Value(Inst* value) : type(Type::Opaque) {
    inner.inst = value;
}

Value::Value(A32::Reg value) : type(Type::A32Reg) {
    inner.imm_a32regref = value;
}

Value::Value(A32::ExtReg value) : type(Type::A32ExtReg) {
    inner.imm_a32extregref = value;
}

Value::Value(A64::Reg value) : type(Type::A64Reg) {
    inner.imm_a64regref = value;
}

Value::Value(A64::Vec value) : type(Type::A64Vec) {
    inner.imm_a64vecref = value;
}

Value::Value(bool value) : type(Type::U1) {
    inner.imm_u1 = value;
}

Value::Value(u8 value) : type(Type::U8) {
    inner.imm_u8 = value;
}

Value::Value(u16 value) : type(Type::U16) {
    inner.imm_u16 = value;
}

Value::Value(u32 value) : type(Type::U32) {
    inner.imm_u32 = value;
}

Value::Value(u64 value) : type(Type::U64) {
    inner.imm_u64 = value;
}

Value::Value(std::array<u8, 8> value) : type(Type::CoprocInfo) {
    inner.imm_coproc = value;
}

Value::Value(Cond value) : type(Type::Cond) {
    inner.imm_cond = value;
}

Value::Value(Chip8::Reg value) : type(Type::Chip8Reg) {
	inner.imm_chip8regref = value;
}

bool Value::IsImmediate() const {
    if (type == Type::Opaque)
        return inner.inst->GetOpcode() == Opcode::Identity ? inner.inst->GetArg(0).IsImmediate() : false;
    return true;
}

bool Value::IsEmpty() const {
    return type == Type::Void;
}

Type Value::GetType() const {
    if (type == Type::Opaque) {
        if (inner.inst->GetOpcode() == Opcode::Identity) {
            return inner.inst->GetArg(0).GetType();
        } else {
            return inner.inst->GetType();
        }
    }
    return type;
}

A32::Reg Value::GetA32RegRef() const {
    ASSERT(type == Type::A32Reg);
    return inner.imm_a32regref;
}

A32::ExtReg Value::GetA32ExtRegRef() const {
    ASSERT(type == Type::A32ExtReg);
    return inner.imm_a32extregref;
}

A64::Reg Value::GetA64RegRef() const {
    ASSERT(type == Type::A64Reg);
    return inner.imm_a64regref;
}

A64::Vec Value::GetA64VecRef() const {
    ASSERT(type == Type::A64Vec);
    return inner.imm_a64vecref;
}

Inst* Value::GetInst() const {
    ASSERT(type == Type::Opaque);
    return inner.inst;
}

bool Value::GetU1() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU1();
    ASSERT(type == Type::U1);
    return inner.imm_u1;
}

u8 Value::GetU8() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU8();
    ASSERT(type == Type::U8);
    return inner.imm_u8;
}

u16 Value::GetU16() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU16();
    ASSERT(type == Type::U16);
    return inner.imm_u16;
}

u32 Value::GetU32() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU32();
    ASSERT(type == Type::U32);
    return inner.imm_u32;
}

u64 Value::GetU64() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU64();
    ASSERT(type == Type::U64);
    return inner.imm_u64;
}

std::array<u8, 8> Value::GetCoprocInfo() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetCoprocInfo();
    ASSERT(type == Type::CoprocInfo);
    return inner.imm_coproc;
}

Cond Value::GetCond() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetCond();
    ASSERT(type == Type::Cond);
    return inner.imm_cond;
}

Chip8::Reg Value::GetChip8RegRef() const {
	ASSERT(type == Type::Chip8Reg);
	return inner.imm_chip8regref;
}

} // namespace Dynarmic::IR
