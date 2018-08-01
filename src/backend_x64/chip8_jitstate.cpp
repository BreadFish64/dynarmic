#include "backend_x64/chip8_jitstate.h"
#include "backend_x64/block_of_code.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_types.h"
#include "frontend/Chip8/location_descriptor.h"

namespace Dynarmic::BackendX64 {

	void Chip8JitState::ResetRSB() {
		rsb_location_descriptors.fill(0xFFFFFFFFFFFFFFFFull);
		rsb_codeptrs.fill(0);
	}

	u64 Chip8JitState::GetUniqueHash() const {
		return static_cast<u64>(Reg[20]) << 32;
	}

} // namespace Dynarmic::BackendX64
