#include "pch.h"
//
//#include <PortableExecutable.h>
//#include <ModuleWrapper.h>
//#include <CoutHelper.h>
//#include <StringWrapper.h>
//
//PortableExecutable::PortableExecutable(ModuleWrapper moduleWrapper) : _buffer(std::move(moduleWrapper.Data())), _moduleBaseAddress(moduleWrapper.BaseAddress())
//{
//	_bufferIt = _buffer.begin();
//
//	if (!IsImageFile())
//	{
//		throw std::runtime_error("File is not of PE image type.");
//	}
//
//	CreateDOSHeader();
//	CreateFileHeader();
//	CreateOptionalHeader();
//	CreateSectionsHeaders();
//	CreateTextSection();
//	CreatePDataSection();
//	CreateStrings();
//}
//
//PortableExecutable::~PortableExecutable()
//{
//}
//
//void PortableExecutable::CreateDOSHeader()
//{
//	if (!CheckBounds(sizeof(IMAGE_DOS_HEADER)))
//	{
//		throw std::runtime_error("Not enough bytes for IMAGE_DOS_HEADER.");
//	}
//
//	_dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(std::to_address(_bufferIt));
//
//	// Move to PE header and skip signature
//	_bufferIt += _dosHeader->e_lfanew + sizeof(LONG);
//}
//
//void PortableExecutable::CreateFileHeader()
//{
//	if (!CheckBounds(sizeof(IMAGE_FILE_HEADER)))
//	{
//		throw std::runtime_error("Not enough bytes for IMAGE_FILE_HEADER.");
//	}
//
//	_fileHeader = reinterpret_cast<IMAGE_FILE_HEADER*>(std::to_address(_bufferIt));
//
//	_bufferIt += sizeof(IMAGE_FILE_HEADER);
//}
//
//void PortableExecutable::CreateOptionalHeader()
//{
//	if (!CheckBounds(sizeof(IMAGE_OPTIONAL_HEADER)))
//	{
//		throw std::runtime_error("Not enough bytes for IMAGE_OPTIONAL_HEADER.");
//	}
//
//	_optionalHeader = reinterpret_cast<IMAGE_OPTIONAL_HEADER*>(std::to_address(_bufferIt));
//
//	if (_optionalHeader->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC && _optionalHeader->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
//	{
//		throw std::runtime_error("Optional header magic value not found.");
//	}
//
//	_bufferIt += sizeof(IMAGE_OPTIONAL_HEADER);
//}
//
//void PortableExecutable::CreateSectionsHeaders()
//{
//	if (!CheckBounds(sizeof(IMAGE_SECTION_HEADER) * _fileHeader->NumberOfSections))
//	{
//		throw std::runtime_error("Not enough bytes for all IMAGE_SECTION_HEADERS.");
//	}
//
//	_sectionsHeaders.reserve(_fileHeader->NumberOfSections);
//	for (WORD i = 0; i < _fileHeader->NumberOfSections; i++)
//	{
//		_sectionsHeaders.push_back(reinterpret_cast<IMAGE_SECTION_HEADER*>(std::to_address(_bufferIt)));
//
//		_bufferIt += sizeof(IMAGE_SECTION_HEADER);
//	}
//
//	ptrdiff_t currentSize = _bufferIt - _buffer.begin();
//	
//	size_t bufferItOffset = (currentSize + _optionalHeader->SectionAlignment - 1) / _optionalHeader->SectionAlignment * _optionalHeader->SectionAlignment;
//
//	if (bufferItOffset > _buffer.size())
//	{
//		throw std::runtime_error("Not enough bytes to align section headers.");
//	}
//
//	_bufferIt = _buffer.begin() + bufferItOffset;
//}
//
//void PortableExecutable::CreateTextSection()
//{
//	IMAGE_SECTION_HEADER* textSectionHeader = FindSectionHeader(".text");
//
//	_textSection.reserve(textSectionHeader->Misc.VirtualSize);
//
//	auto textSectionStartIt = _buffer.begin() + textSectionHeader->VirtualAddress;
//	auto textSectionEndIt = textSectionStartIt + textSectionHeader->Misc.VirtualSize;
//
//	_textSection.reserve(textSectionEndIt - textSectionStartIt);
//	for (textSectionStartIt; textSectionStartIt != textSectionEndIt; textSectionStartIt++)
//	{
//		_textSection.push_back(std::to_address(textSectionStartIt));
//	}
//}
//
//void PortableExecutable::CreatePDataSection()
//{
//	IMAGE_SECTION_HEADER* pdataSectionHeader = FindSectionHeader(".pdata");
//
//	auto pdataSectionStartIt = _buffer.begin() + pdataSectionHeader->VirtualAddress;
//	auto pdataSectionEndIt = pdataSectionStartIt + pdataSectionHeader->Misc.VirtualSize;
//
//	_functionEntries.reserve(pdataSectionHeader->Misc.VirtualSize / sizeof(RUNTIME_FUNCTION));
//	for (pdataSectionStartIt; pdataSectionStartIt != pdataSectionEndIt; pdataSectionStartIt += sizeof(RUNTIME_FUNCTION))
//	{
//		_functionEntries.push_back(reinterpret_cast<RUNTIME_FUNCTION*>(std::to_address(pdataSectionStartIt)));
//	}
//}
//
//void PortableExecutable::CreateStrings()
//{
//	IMAGE_SECTION_HEADER* rdataSectionHeader = FindSectionHeader(".rdata");
//
//	auto rdataSectionStartIt = _buffer.begin() + rdataSectionHeader->VirtualAddress;
//
//	_strings.clear();
//	for (size_t i = 0; i < rdataSectionHeader->Misc.VirtualSize; i++)
//	{
//		unsigned char* current = reinterpret_cast<unsigned char*>(std::to_address(rdataSectionStartIt + i));
//		if (std::isprint(*current))
//		{
//			auto stringStartIt = rdataSectionStartIt + i;
//			size_t size = 0;
//			while (*current != '\0' && std::isprint(*current))
//			{
//				current = reinterpret_cast<unsigned char*>(std::to_address(rdataSectionStartIt + ++i));
//				size++;
//			}
//
//			if (size >= 4)
//			{
//				_strings.push_back(StringWrapper{ std::to_address(stringStartIt), size, });
//			}
//		}
//	}
//}
//
//ptrdiff_t PortableExecutable::GetStringRelativeOffset(const std::string& targetString) const
//{
//	auto sumInputStringWrapperIt = std::find_if(_strings.begin(), _strings.end(), [targetString](const StringWrapper& stringWrapper)
//		{
//			return std::string_view(reinterpret_cast<char*>(stringWrapper.start), stringWrapper.size) == targetString;
//		});
//
//	if (sumInputStringWrapperIt == _strings.end())
//	{
//		throw std::runtime_error(std::format("Unable to find '{0}' string.", targetString));
//	}
//
//	StringWrapper sumInputStringWrapper = *sumInputStringWrapperIt;
//
//	ptrdiff_t inputSumStringRelativeOffset = sumInputStringWrapper.start - _buffer.data();
//
//	return inputSumStringRelativeOffset;
//}
//
//void PortableExecutable::Foo(HANDLE pH)
//{
//	constexpr std::array leaOpCode = { BYTE(0x48), BYTE(0x8D), BYTE(0x15) };
//
//	ptrdiff_t offset = GetStringRelativeOffset("Inside sum");
//
//	for (size_t i = 0; i < _textSection.size() - 3; i++)
//	{
//		if (std::ranges::equal(leaOpCode.begin(), leaOpCode.end(), _textSection[i], _textSection[i + 3]))
//		{
//			int32_t leaOffset = *reinterpret_cast<int32_t*>(_textSection[i + 3]);
//
//			if (_textSection[i + 7] + leaOffset == std::to_address(_buffer.begin() + offset))
//			{
//				ptrdiff_t t = GetStringRelativeOffset("Enter second number: ");
//				ptrdiff_t a = _textSection[i + 3] - _buffer.data();
//				auto p = t - (_textSection[i + 7] - _buffer.data());
//				if (!WriteProcessMemory(pH, _moduleBaseAddress + a, &p, 4, nullptr))
//				{
//					throw std::runtime_error("Throwwwwwwwsss");
//				}
//			}
//		}
//	}
//}
//
//bool PortableExecutable::IsImageFile() const
//{
//	constexpr std::array peImageSignature = { BYTE(0x50), BYTE(0x45), BYTE(0x00), BYTE(0x00) };
//
//	if (!CheckBounds(sizeof(IMAGE_DOS_HEADER)))
//	{
//		throw std::runtime_error("Not enough bytes to find PE header offset.");
//	}
//
//	LONG* peImageOffset = reinterpret_cast<LONG*>(std::to_address(_bufferIt) + 0x3C);
//
//	if (!CheckBounds(*peImageOffset + sizeof(4)))
//	{
//		throw std::runtime_error("Invalid PE header offset.");
//	}
//
//	return std::ranges::equal(peImageSignature.begin(), peImageSignature.end(), _bufferIt + *peImageOffset, _bufferIt + *peImageOffset + sizeof(4));
//}
//
//IMAGE_SECTION_HEADER* PortableExecutable::FindSectionHeader(const char* name)
//{
//	auto rdataSectionHeaderIt = std::find_if(_sectionsHeaders.begin(), _sectionsHeaders.end(), [name](const IMAGE_SECTION_HEADER* sectionHeader)
//		{
//			char nameDestination[9] = {};
//			std::memcpy(nameDestination, sectionHeader->Name, sizeof(sectionHeader->Name));
//			return std::strcmp(nameDestination, name) == 0;
//		});
//
//	if (rdataSectionHeaderIt == _sectionsHeaders.end())
//	{
//		throw std::runtime_error(std::format("Unable to find {} section header.", name));
//	}
//
//	return *rdataSectionHeaderIt;
//}
//
//bool PortableExecutable::CheckBounds(size_t size) const
//{
//	return _buffer.end() - _bufferIt >= static_cast<ptrdiff_t>(size);
//}
