#include <pch.h>

#include <CodeBuilder.h>
#include <bit>
#include <CoutHelper.h>
#include <BytesHelper.h>

namespace Builders
{
	CodeBuilder::CodeBuilder(const Models::PortableExecutable& portableExecutable) : _portableExecutable(portableExecutable)
	{
	}

	CodeBuilder::~CodeBuilder()
	{
	}

	/// <summary>
	/// Creates a call site detour by replacing a call instruction with a trampoline that redirects execution to a target function.
	/// </summary>
	/// <param name="targetFunctionAddress">The memory address of the function to redirect execution to.</param>
	/// <returns>A Trampoline object containing the new call instruction bytes, trampoline bytes, and target function address bytes along with their respective memory addresses.</returns>
	Models::Trampoline CodeBuilder::CreateCallSiteDetour(std::uintptr_t targetFunctionAddress)
	{
		const std::uintptr_t baseAddress = _portableExecutable.GetBaseAddress();
		std::span<const std::byte> buffer = _portableExecutable.GetBuffer();

		// Call instruction(E8) to replace.
		// TODO: This needs to be generalized, somehow.
		constexpr std::array callInstruction = { std::byte(0xE8), std::byte(0x54), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF) };

		// Find address of the call instruction(E8) so we know where to write new call instruction to.
		std::span<const std::byte>::iterator it = std::search(buffer.begin(), buffer.end(), callInstruction.begin(), callInstruction.end());

		if (it == buffer.end())
		{
			throw std::runtime_error("Unable to find specified call instruction");
		}

		// Create code cave big enough to store trampoline instruction(FF /4) and address of target function.
		// FF /2 requires 6 bytes and target function address requires 8 bytes.
		std::uint32_t caveOffset = FindCodeCave(14);
		std::uint32_t trampolineOffset = caveOffset;
		std::uint32_t targetFunctionAddressOffset = caveOffset + 6;

		// Calculate address of the new call instruction bytes.
		std::uintptr_t callAddress = baseAddress + std::distance(buffer.begin(), it);

		// Create new call instruction to jump to cave.
		// Call instruction(E8) is RIP-relative, meaning we need to find offset between current RIP value and location of trampoline.
		// E8 is 5 bytes long, so subtract 5 bytes.
		std::vector<std::byte> newCallInstructionBytes = Helpers::ToBytes(static_cast<std::int32_t>(baseAddress + trampolineOffset - callAddress - 5));
		newCallInstructionBytes.insert(newCallInstructionBytes.begin(), std::byte(0xE8));

		// Create target function address
		std::vector<std::byte> targetFunctionAddressByte = Helpers::ToBytes(targetFunctionAddress);

		// Calculate address of target function bytes.
		std::uintptr_t targetFunctionAddressBytesAddress = baseAddress + targetFunctionAddressOffset;

		// Create trampoline bytes.
		// Indirect jump instruction(FF /4) is RIP-relative, meaning we need to find offset between current RIP value and memory location of memory that holds 
		// address of the target function.
		// In our case memory is directly after the instruction, and since RIP is moved before instruction, we can just set target address to 0.
		std::vector<std::byte> trampolineBytes = Helpers::ToBytes(0);
		trampolineBytes.insert(trampolineBytes.begin(), { std::byte(0xFF), std::byte(0x25) });

		// Calculate address of trampoline.
		std::uintptr_t trampolineAddress = baseAddress + trampolineOffset;

		return Models::Trampoline(newCallInstructionBytes, callAddress, trampolineBytes, trampolineAddress, targetFunctionAddressByte, targetFunctionAddressBytesAddress);
	}

	/// <summary>
	/// Searches for a code cave (sequence of null bytes) of the specified size in the executable's text section.
	/// </summary>
	/// <param name="size">The size in bytes of the code cave to find.</param>
	/// <returns>The offset in the buffer where the code cave was found. Throws std::runtime_error if no suitable code cave is found.</returns>
	std::uint32_t CodeBuilder::FindCodeCave(int size) const
	{
		std::span<const std::byte> buffer = _portableExecutable.GetBuffer();
		const IMAGE_SECTION_HEADER* textSectionHeader = _portableExecutable.GetFileImageSectionHeader();

		// Calculate start and end of text section
		std::span<const std::byte>::iterator startIt = buffer.begin() + textSectionHeader->VirtualAddress;
		std::span<const std::byte>::iterator endIt = startIt + textSectionHeader->SizeOfRawData;

		std::vector<std::byte> cave(size, std::byte(0x00));
		std::span<const std::byte>::iterator it = std::search(startIt, endIt, cave.begin(), cave.end());

		if (it == endIt)
		{
			throw std::runtime_error("Unable to find code cave of specified size in text section");
		}

		// Size of portable executable(both PE32 and PE32+) is limited to 2GB, meaning offset can never be bigger then 32 bits.
		return static_cast<std::uint32_t>(std::distance(buffer.begin(), it));
	}
}