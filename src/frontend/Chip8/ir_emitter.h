#pragma once

#include <initializer_list>

#include "common/common_types.h"
#include "frontend/Chip8/location_descriptor.h"
#include "frontend/Chip8/types.h"
#include "frontend/ir/ir_emitter.h"
#include "frontend/ir/value.h"

namespace Dynarmic::Chip8 {

	enum class Exception;

	/**
	* Convenience class to construct a basic block of the intermediate representation.
	* `block` is the resulting block.
	* The user of this class updates `current_location` as appropriate.
	*/
	class IREmitter : public IR::IREmitter {
	public:
		explicit IREmitter(IR::Block& block, LocationDescriptor descriptor) : IR::IREmitter(block), current_location(descriptor) {}

		LocationDescriptor current_location;

		u32 PC();
		u32 AlignPC(size_t alignment);

		IR::U32 GetRegister(Reg source_reg);
		void SetRegister(const Reg dest_reg, const IR::U32& value);

		void WritePC(const IR::U32& value);

		void CallSupervisor(const IR::U32& value);

		IR::U8 ReadMemory8(const IR::U32& vaddr);
		IR::U16 ReadMemory16(const IR::U32& vaddr);
		void WriteMemory8(const IR::U32& vaddr, const IR::U8& value);
		void WriteMemory16(const IR::U32& vaddr, const IR::U16& value);
	};

} // namespace Dynarmic::Chip8
