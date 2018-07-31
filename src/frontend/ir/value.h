/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <type_traits>

#include "common/assert.h"
#include "common/common_types.h"
#include "frontend/A32/types.h"
#include "frontend/A64/types.h"
#include "frontend/Chip8/types.h"
#include "frontend/ir/cond.h"
#include "frontend/ir/type.h"

namespace Dynarmic::IR {

class Inst;

/**
 * A representation of a value in the IR.
 * A value may either be an immediate or the result of a microinstruction.
 */
class Value {
public:
    Value() : type(Type::Void) {}
    explicit Value(Inst* value);
    explicit Value(A32::Reg value);
    explicit Value(A32::ExtReg value);
    explicit Value(A64::Reg value);
    explicit Value(A64::Vec value);
    explicit Value(bool value);
    explicit Value(u8 value);
    explicit Value(u16 value);
    explicit Value(u32 value);
    explicit Value(u64 value);
    explicit Value(std::array<u8, 8> value);
    explicit Value(Cond value);
	explicit Value(Chip8::Reg value);

    bool IsEmpty() const;
    bool IsImmediate() const;
    Type GetType() const;

    Inst* GetInst() const;
    A32::Reg GetA32RegRef() const;
    A32::ExtReg GetA32ExtRegRef() const;
    A64::Reg GetA64RegRef() const;
    A64::Vec GetA64VecRef() const;
    bool GetU1() const;
    u8 GetU8() const;
    u16 GetU16() const;
    u32 GetU32() const;
    u64 GetU64() const;
    std::array<u8, 8> GetCoprocInfo() const;
    Cond GetCond() const;
	Chip8::Reg GetChip8RegRef() const;

private:
    Type type;

    union {
        Inst* inst; // type == Type::Opaque
        A32::Reg imm_a32regref;
        A32::ExtReg imm_a32extregref;
        A64::Reg imm_a64regref;
        A64::Vec imm_a64vecref;
        bool imm_u1;
        u8 imm_u8;
        u16 imm_u16;
        u32 imm_u32;
        u64 imm_u64;
        std::array<u8, 8> imm_coproc;
        Cond imm_cond;
		Chip8::Reg imm_chip8regref;
    } inner;
};
static_assert(sizeof(Value) <= 2 * sizeof(u64), "IR::Value should be kept small in size");

template <Type type_>
class TypedValue final : public Value {
public:
    TypedValue() = default;

    template <Type other_type, typename = std::enable_if_t<(other_type & type_) != Type::Void>>
    /* implicit */ TypedValue(const TypedValue<other_type>& value) : Value(value) {
        ASSERT((value.GetType() & type_) != Type::Void);
    }

    explicit TypedValue(const Value& value) : Value(value) {
        ASSERT((value.GetType() & type_) != Type::Void);
    }
};

using U1 = TypedValue<Type::U1>;
using U8 = TypedValue<Type::U8>;
using U16 = TypedValue<Type::U16>;
using U32 = TypedValue<Type::U32>;
using U64 = TypedValue<Type::U64>;
using U128 = TypedValue<Type::U128>;
using U32U64 = TypedValue<Type::U32 | Type::U64>;
using UAny = TypedValue<Type::U8 | Type::U16 | Type::U32 | Type::U64>;
using UAnyU128 = TypedValue<Type::U8 | Type::U16 | Type::U32 | Type::U64 | Type::U128>;
using NZCV = TypedValue<Type::NZCVFlags>;

} // namespace Dynarmic::IR
