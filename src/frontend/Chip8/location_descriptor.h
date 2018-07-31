#pragma once

#include "common/common_types.h"
#include "frontend/ir/location_descriptor.h"
#include <functional>
#include <iosfwd>
#include <tuple>

namespace Dynarmic::Chip8 {

	/**
	* LocationDescriptor describes the location of a basic block.
	* The location is not solely based on the PC because other flags influence the way
	* instructions should be translated. The CPSR.T flag is most notable since it
	* tells us if the processor is in Thumb or Arm mode.
	*/
	class LocationDescriptor {
	public:
		LocationDescriptor(u32 chip8_pc)
			: chip8_pc(chip8_pc){}

		explicit LocationDescriptor(const IR::LocationDescriptor& o) {
			chip8_pc = o.Value() >> 32;
		}

		u32 PC() const { return chip8_pc; }

		bool operator == (const LocationDescriptor& o) const {
			return chip8_pc == o.chip8_pc;
		}

		bool operator != (const LocationDescriptor& o) const {
			return !operator==(o);
		}

		LocationDescriptor SetPC(u32 new_chip8_pc) const {
			return LocationDescriptor(new_chip8_pc);
		}

		LocationDescriptor AdvancePC(int amount) const {
			return LocationDescriptor(chip8_pc + amount);
		}

		u64 UniqueHash() const {
			// This value MUST BE UNIQUE.
			// This calculation has to match up with EmitX64::EmitTerminalPopRSBHint
			u64 pc_u64 = u64(chip8_pc) << 32;
			return pc_u64;
		}

		operator IR::LocationDescriptor() const {
			return IR::LocationDescriptor{ UniqueHash() };
		}

	private:
		u32 chip8_pc;       ///< Current program counter value.
	};

	/**
	* Provides a string representation of a LocationDescriptor.
	*
	* @param o          Output stream
	* @param descriptor The descriptor to get a string representation of
	*/
	std::ostream& operator<<(std::ostream& o, const LocationDescriptor& descriptor);

} // namespace Dynarmic::A32

namespace std {
	template <>
	struct less<Dynarmic::Chip8::LocationDescriptor> {
		bool operator()(const Dynarmic::Chip8::LocationDescriptor& x, const Dynarmic::Chip8::LocationDescriptor& y) const {
			return x.UniqueHash() < y.UniqueHash();
		}
	};
	template <>
	struct hash<Dynarmic::Chip8::LocationDescriptor> {
		size_t operator()(const Dynarmic::Chip8::LocationDescriptor& x) const {
			return std::hash<u64>()(x.UniqueHash());
		}
	};
}
