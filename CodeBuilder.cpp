#include <pch.h>

#include <CodeBuilder.h>
#include <bit>

namespace Builders
{
	CodeBuilder::CodeBuilder(const Models::PortableExecutable& portableExecutable) : _portableExecutable(portableExecutable)
	{
		CreateTrampoline(2);
	}

	CodeBuilder::~CodeBuilder()
	{
	}

	Models::Trampoline CodeBuilder::CreateTrampoline(std::uintptr_t targetFunction)
	{
		std::span<const std::byte> buffer = _portableExecutable.GetBuffer();

		// Call instruction(E8) to replace.
		constexpr std::array callInstruction = { std::byte(0xE8), std::byte(0x54), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF) };

		// Find address of the call instruction(E8) so we know where to write new call instruction to.
		std::span<const std::byte>::iterator it = std::search(buffer.begin(), buffer.end(), callInstruction.begin(), callInstruction.end());

		if (it == buffer.end())
		{
			throw std::runtime_error("Unable to find specified call instruction");
		}

		std::size_t caveOffset = FindCodeCave(6);

		// Convert caveOffset to array of bytes.
		// Since call instruction(E8) is calculated based on RIP to next instruction, and call instruction has 5 bytes, we subtract those 5 bytes.
		std::array<std::byte, sizeof(size_t)> bytes = std::bit_cast<std::array<std::byte, sizeof(size_t)>>(caveOffset - 5);

		// Create new call instruction to jump to cave.
		std::uintptr_t callAddress = reinterpret_cast<std::uintptr_t>(std::to_address(it));
		std::vector<std::byte> newCallInstruction(5);
		newCallInstruction[0] = std::byte(0xE8);
		for (size_t i = 0; i < 4; i++)
		{
			newCallInstruction[i + 1] = bytes[i];
		}

		return Models::Trampoline();
		//return Models::Trampoline(newCallInstruction, callAddress, );
	}

	/// <summary>
	/// Searches for a code cave (sequence of null bytes) of the specified size in the executable's text section.
	/// </summary>
	/// <param name="size">The size in bytes of the code cave to find.</param>
	/// <returns>The offset in the buffer where the code cave was found. Throws std::runtime_error if no suitable code cave is found.</returns>
	std::size_t CodeBuilder::FindCodeCave(int size) const
	{
		std::span<const std::byte> textSection = _portableExecutable.GetTextSection();
		std::span<const std::byte> buffer = _portableExecutable.GetBuffer();

		std::vector<std::byte> cave(size, std::byte(0x00));
		std::span<const std::byte>::iterator it = std::search(textSection.begin(), textSection.end(), cave.begin(), cave.end());

		if (it == textSection.end())
		{
			throw std::runtime_error("Unable to find code cave of specified size in text section");
		}

		return std::to_address(it) - buffer.data();
	}
}