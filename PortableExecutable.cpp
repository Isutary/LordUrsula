#include <iostream>
#include <iomanip>
#include <algorithm>
#include <array>
#include <cstring>

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
	CreateSectionsHeaders();
	CreateTextSection();
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

void PortableExecutable::CreateSectionsHeaders()
{
	if (!CheckBounds(sizeof(IMAGE_SECTION_HEADER) * _fileHeader->NumberOfSections))
	{
		throw std::runtime_error("Not enough bytes for all IMAGE_SECTION_HEADERS.");
	}

	_sectionsHeaders.reserve(_fileHeader->NumberOfSections);
	for (WORD i = 0; i < _fileHeader->NumberOfSections; i++)
	{
		_sectionsHeaders.push_back(reinterpret_cast<IMAGE_SECTION_HEADER*>(std::to_address(_bufferIt)));

		_bufferIt += sizeof(IMAGE_SECTION_HEADER);
	}

	ptrdiff_t currentSize = _bufferIt - _buffer.begin();
	
	size_t bufferItOffset = (currentSize + _optionalHeader->SectionAlignment - 1) / _optionalHeader->SectionAlignment * _optionalHeader->SectionAlignment;

	if (bufferItOffset > _buffer.size())
	{
		throw std::runtime_error("Not enough bytes to align section headers.");
	}

	_bufferIt = _buffer.begin() + bufferItOffset;
}

void PortableExecutable::CreateTextSection()
{
	auto textSectionHeaderIt = std::find_if(_sectionsHeaders.begin(), _sectionsHeaders.end(), [](const IMAGE_SECTION_HEADER* sectionHeader)
		{
			char nameDestination[9] = {};
			std::memcpy(nameDestination, sectionHeader->Name, sizeof(sectionHeader->Name));
			return std::strcmp(nameDestination, ".text") == 0;
		});

	if (textSectionHeaderIt == _sectionsHeaders.end())
	{
		throw std::runtime_error("Unable to find .text section header.");
	}

	IMAGE_SECTION_HEADER* textSectionHeader = *textSectionHeaderIt;

	_textSection.reserve(textSectionHeader->Misc.VirtualSize);

	auto it = _buffer.begin() + textSectionHeader->VirtualAddress;

	for (it; it < _buffer.begin() + textSectionHeader->VirtualAddress + textSectionHeader->Misc.VirtualSize; it++)
	{
		if (static_cast<int>(*it) == 0x8D && static_cast<int>(*(it + 1)) == 0x04 && static_cast<int>(*(it + 2)) == 0x3B)
		{
			break;
		}
	}

	std::cout << "Result: " << std::hex << static_cast<int>(*it) << std::endl;
}

bool PortableExecutable::IsImageFile() const
{
	constexpr std::array peImageSignature = {std::byte(0x50), std::byte(0x45), std::byte(0x00), std::byte(0x00)};

	if (!CheckBounds(sizeof(IMAGE_DOS_HEADER)))
	{
		throw std::runtime_error("Not enough bytes to find PE header offset.");
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
