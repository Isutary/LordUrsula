#pragma once

#include <vector>
#include <span>

class ModuleWrapper
{
public:
	ModuleWrapper(std::vector<std::byte> buffer, std::uintptr_t baseAddress) : _buffer(std::move(buffer)), _baseAddress(baseAddress) {};
	ModuleWrapper(const ModuleWrapper& other) = delete;
	ModuleWrapper(ModuleWrapper&& other) { std::cout << "MOVE CTRO " << _buffer.size() << std::endl; };
	ModuleWrapper& operator=(const ModuleWrapper& other) = delete;
	ModuleWrapper& operator=(ModuleWrapper&& other) = default;
	std::span<const std::byte> const Data() const noexcept { return _buffer; };
	std::span<std::byte> Data() noexcept { return _buffer; };
	std::uintptr_t BaseAddress() const noexcept { return _baseAddress; };
	size_t Size() const noexcept { return _buffer.size(); };
private:
	std::vector<std::byte> _buffer;
	std::uintptr_t _baseAddress;
};
