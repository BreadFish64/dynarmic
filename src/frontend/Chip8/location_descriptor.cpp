#include "frontend/Chip8/location_descriptor.h"
#include <fmt/format.h>
#include <ostream>

namespace Dynarmic::Chip8 {

	std::ostream& operator<<(std::ostream& o, const LocationDescriptor& loc) {
		o << fmt::format("{}",
			loc.PC());
		return o;
	}

} // namespace Dynarmic::Chip8
