#pragma once

#include <array>

#include <xbyak.h>

#include "common/common_types.h"

namespace Dynarmic::BackendX64 {

	class BlockOfCode;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4324) // Structure was padded due to alignment specifier
#endif

	struct Chip8JitState {
		using ProgramCounterType = u32;

		Chip8JitState() { ResetRSB(); }

		std::array<u32, 20> Reg{}; // Current register file.
								   // TODO: Mode-specific register sets unimplemented.

		static constexpr size_t SpillCount = 64;
		std::array<u64, SpillCount> Spill{}; // Spill.
		static Xbyak::Address GetSpillLocationFromIndex(size_t i) {
			using namespace Xbyak::util;
			return qword[r15 + offsetof(Chip8JitState, Spill) + i * sizeof(u64)];
		}

		// For internal use (See: BlockOfCode::RunCode)
		u32 guest_MXCSR = 0x00001f80;
		u32 save_host_MXCSR = 0;
		s64 cycles_to_run = 0;
		s64 cycles_remaining = 0;
		bool halt_requested = false;

		// Exclusive state
		static constexpr u32 RESERVATION_GRANULE_MASK = 0xFFFFFFF8;
		u32 exclusive_state = 0;
		u32 exclusive_address = 0;

		static constexpr size_t RSBSize = 16; // MUST be a power of 2.
		static constexpr size_t RSBPtrMask = RSBSize - 1;
		u32 rsb_ptr = 0;
		std::array<u64, RSBSize> rsb_location_descriptors;
		std::array<u64, RSBSize> rsb_codeptrs;
		void ResetRSB();

		u64 GetUniqueHash() const;
	};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

	using CodePtr = const void*;

} // namespace Dynarmic::BackendX64
