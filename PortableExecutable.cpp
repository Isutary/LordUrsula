#include <iostream>
#include <iomanip>
#include <algorithm>
#include <array>

#include <PortableExecutable.h>

PortableExecutable::PortableExecutable(std::vector<std::byte> buffer) : _buffer(std::move(buffer))
{
	_bufferIt = _buffer.begin();

	if (!IsImageFile())
	{
		throw std::runtime_error("File is not of PE image type.");
	}

	CreateDOSHeader();
	CreateFileHeader();
	CreateOptionalHeader();
}

PortableExecutable::~PortableExecutable()
{
}

void PortableExecutable::CreateDOSHeader()
{
	if (!CheckBounds(sizeof(IMAGE_DOS_HEADER)))
	{
		throw std::runtime_error("Not enough bytes for IMAGE_DOS_HEADER.");
	}

	_dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(std::to_address(_bufferIt));

	// Move to PE header and skip signature
	_bufferIt += _dosHeader->e_lfanew + sizeof(LONG);
}

void PortableExecutable::CreateFileHeader()
{
	if (!CheckBounds(sizeof(IMAGE_FILE_HEADER)))
	{
		throw std::runtime_error("Not enough bytes for IMAGE_FILE_HEADER.");
	}

	_fileHeader = reinterpret_cast<IMAGE_FILE_HEADER*>(std::to_address(_bufferIt));

	_bufferIt += sizeof(IMAGE_FILE_HEADER);
}

void PortableExecutable::CreateOptionalHeader()
{
	if (!CheckBounds(sizeof(IMAGE_OPTIONAL_HEADER)))
	{
		throw std::runtime_error("Not enough bytes for IMAGE_OPTIONAL_HEADER.");
	}

	_optionalHeader = reinterpret_cast<IMAGE_OPTIONAL_HEADER*>(std::to_address(_bufferIt));

	if (_optionalHeader->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC && _optionalHeader->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		throw std::runtime_error("Optional header magic value not found.");
	}

	_bufferIt += sizeof(IMAGE_OPTIONAL_HEADER);
}

bool PortableExecutable::IsImageFile() const
{
	constexpr std::array peImageSignature = {std::byte(0x50), std::byte(0x45), std::byte(0x00), std::byte(0x00)};

	if (!CheckBounds(sizeof(IMAGE_DOS_HEADER)))
	{
		throw std::runtime_error("Buffer doesn't contain enough bytes to find PE header offset.");
	}

	LONG* peImageOffset = reinterpret_cast<LONG*>(std::to_address(_bufferIt) + 0x3C);

	if (*peImageOffset < 0 || !CheckBounds(*peImageOffset + sizeof(LONG)))
	{
		throw std::runtime_error("Invalid PE header offset.");
	}

	return std::ranges::equal(peImageSignature.begin(), peImageSignature.end(), _bufferIt + *peImageOffset, _bufferIt + *peImageOffset + sizeof(LONG));
}

bool PortableExecutable::CheckBounds(size_t size) const
{
	return _buffer.end() - _bufferIt >= static_cast<ptrdiff_t>(size);
}
