#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace Dynarmic {
	namespace Chip8 {

		using VAddr = std::uint32_t;

		enum class Exception {
			/// An UndefinedFault occured due to executing instruction with an unallocated encoding
			UndefinedInstruction,
			/// An unpredictable instruction is to be executed. Implementation-defined behaviour should now happen.
			/// This behaviour is up to the user of this library to define.
			UnpredictableInstruction,
			/// A BKPT instruction was executed.
			Breakpoint,
		};

		/// These function pointers may be inserted into compiled code.
		struct UserCallbacks {
			virtual ~UserCallbacks() = default;

			// All reads through this callback are 4-byte aligned.
			// Memory must be interpreted as little endian.
			virtual std::uint32_t MemoryReadCode(VAddr vaddr) { return MemoryRead16(vaddr); }

			// Reads through these callbacks may not be aligned.
			// Memory must be interpreted as if ENDIANSTATE == 0, endianness will be corrected by the JIT.
			virtual std::uint8_t MemoryRead8(VAddr vaddr) = 0;
			virtual std::uint16_t MemoryRead16(VAddr vaddr) = 0;
			virtual std::uint32_t MemoryRead32(VAddr vaddr) = 0;
			virtual std::uint64_t MemoryRead64(VAddr vaddr) = 0;

			// Writes through these callbacks may not be aligned.
			virtual void MemoryWrite8(VAddr vaddr, std::uint8_t value) = 0;
			virtual void MemoryWrite16(VAddr vaddr, std::uint16_t value) = 0;
			virtual void MemoryWrite32(VAddr vaddr, std::uint32_t value) = 0;
			virtual void MemoryWrite64(VAddr vaddr, std::uint64_t value) = 0;

			// If this callback returns true, the JIT will assume MemoryRead* callbacks will always
			// return the same value at any point in time for this vaddr. The JIT may use this information
			// in optimizations.
			// A conservative implementation that always returns false is safe.
			virtual bool IsReadOnlyMemory(VAddr /* vaddr */) { return false; }

			/// The intrepreter must execute exactly num_instructions starting from PC.
			virtual void InterpreterFallback(VAddr pc, size_t num_instructions) = 0;

			// This callback is called whenever a SVC instruction is executed.
			virtual void CallSVC(std::uint32_t swi) = 0;

			virtual void ExceptionRaised(VAddr pc, Exception exception) = 0;

			// Timing-related callbacks
			// ticks ticks have passed
			virtual void AddTicks(std::uint64_t ticks) = 0;
			// How many more ticks am I allowed to execute?
			virtual std::uint64_t GetTicksRemaining() = 0;
		};

		struct UserConfig {
			UserCallbacks* callbacks;

			// Page Table
			// The page table is used for faster memory access. If an entry in the table is nullptr,
			// the JIT will fallback to calling the MemoryRead*/MemoryWrite* callbacks.
			static constexpr std::size_t PAGE_BITS = 12;
			static constexpr std::size_t NUM_PAGE_TABLE_ENTRIES = 1 << (32 - PAGE_BITS);
			std::array<std::uint8_t*, NUM_PAGE_TABLE_ENTRIES>* page_table = nullptr;
		};
	} // namespace A32
} // namespace Dynarmic
