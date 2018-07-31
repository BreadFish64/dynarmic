#pragma once

#include "common/common_types.h"
#include <functional>

namespace Dynarmic::IR {
	class Block;
} // namespace Dynarmic::IR

namespace Dynarmic::Chip8 {

	class LocationDescriptor;

	using MemoryReadCodeFuncType = std::function<u16(u32 vaddr)>;

	/**
	* This function translates instructions in memory into our intermediate representation.
	* @param descriptor The starting location of the basic block. Includes information like PC, Thumb state, &c.
	* @param memory_read_code The function we should use to read emulated memory.
	* @return A translated basic block in the intermediate representation.
	*/
	IR::Block Translate(LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code);

} // namespace Dynarmic::Chip8
