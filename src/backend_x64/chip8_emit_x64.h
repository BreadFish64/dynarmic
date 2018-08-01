#pragma once

#include "backend_x64/block_range_information.h"
#include "backend_x64/chip8_jitstate.h"
#include "backend_x64/emit_x64.h"
#include "dynarmic/Chip8/chip8.h"
#include "dynarmic/Chip8/config.h"
#include "frontend/Chip8/location_descriptor.h"
#include "frontend/ir/terminal.h"
#include <boost/optional.hpp>

namespace Dynarmic::BackendX64 {

	class RegAlloc;

	struct Chip8EmitContext final : public EmitContext {
		Chip8EmitContext(RegAlloc& reg_alloc, IR::Block& block);
		Chip8::LocationDescriptor Location() const;

		FP::RoundingMode FPSCR_RMode() const override;
		u32 FPCR() const override;
		bool FPSCR_FTZ() const override;
		bool FPSCR_DN() const override;
	};

	class Chip8EmitX64 final : public EmitX64 {
	public:
		Chip8EmitX64(BlockOfCode& code, Chip8::UserConfig config, Chip8::Jit* jit_interface);
		~Chip8EmitX64() override;

		/**
		* Emit host machine code for a basic block with intermediate representation `ir`.
		* @note ir is modified.
		*/
		BlockDescriptor Emit(IR::Block& ir);

		void ClearCache() override;

		void InvalidateCacheRanges(const boost::icl::interval_set<u32>& ranges);

	protected:
		const Chip8::UserConfig config;
		Chip8::Jit* jit_interface;
		BlockRangeInformation<u32> block_ranges;

		const void* read_memory_8;
		const void* read_memory_16;
		const void* read_memory_32;
		const void* read_memory_64;
		const void* write_memory_8;
		const void* write_memory_16;
		const void* write_memory_32;
		const void* write_memory_64;
		void GenMemoryAccessors();

		// Microinstruction emitters
#define OPCODE(...)
#define A32OPC(...)
#define A64OPC(...)
#define Chip8OPC(name, type, ...) void EmitChip8##name(Chip8EmitContext& ctx, IR::Inst* inst);
#include "frontend/ir/opcodes.inc"
#undef OPCODE
#undef A32OPC
#undef A64OPC
#undef Chip8OPC

		// Terminal instruction emitters
		void EmitTerminalImpl(IR::Term::Interpret terminal, IR::LocationDescriptor initial_location) override;
		void EmitTerminalImpl(IR::Term::ReturnToDispatch terminal, IR::LocationDescriptor initial_location) override;
		void EmitTerminalImpl(IR::Term::LinkBlock terminal, IR::LocationDescriptor initial_location) override;
		void EmitTerminalImpl(IR::Term::LinkBlockFast terminal, IR::LocationDescriptor initial_location) override;
		void EmitTerminalImpl(IR::Term::PopRSBHint terminal, IR::LocationDescriptor initial_location) override;
		void EmitTerminalImpl(IR::Term::If terminal, IR::LocationDescriptor initial_location) override;
		void EmitTerminalImpl(IR::Term::CheckBit terminal, IR::LocationDescriptor initial_location) override;
		void EmitTerminalImpl(IR::Term::CheckHalt terminal, IR::LocationDescriptor initial_location) override;

		// Patching
		void EmitPatchJg(const IR::LocationDescriptor& target_desc, CodePtr target_code_ptr = nullptr) override;
		void EmitPatchJmp(const IR::LocationDescriptor& target_desc, CodePtr target_code_ptr = nullptr) override;
		void EmitPatchMovRcx(CodePtr target_code_ptr = nullptr) override;
	};

} // namespace Dynarmic::BackendX64
