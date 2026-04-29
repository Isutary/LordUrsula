#pragma once

#include <vector>
#include <span>

namespace Models
{
	class Trampoline
	{
	public:
		Trampoline() {};
		Trampoline(std::vector<std::byte> callInstruction, std::uintptr_t callAddress, std::vector<std::byte> trampolineInstruction, std::uintptr_t trampolineAddress)
			: _callInstruction(callInstruction), _callAddress(callAddress), _trampolineInstruction(trampolineInstruction), _trampolineAddress(trampolineAddress) {};
		std::vector<std::byte> GetCallInstruction() const { return _callInstruction; }
		std::uintptr_t GetCallAddress() const { return _callAddress; }
		std::vector<std::byte> GetTrampolineInstruction() const { return _trampolineInstruction; }
		std::uintptr_t GetTrampolineAddress() const { return _trampolineAddress; }
	private:
		std::vector<std::byte> _callInstruction;
		std::uintptr_t _callAddress;
		std::vector<std::byte> _trampolineInstruction;
		std::uintptr_t _trampolineAddress;
	};
}
