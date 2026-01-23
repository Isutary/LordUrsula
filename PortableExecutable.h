#pragma once

#include "pch.h"

#include <ModuleWrapper.h>
#include <StringWrapper.h>

class PortableExecutable
{
public:
	PortableExecutable(ModuleWrapper moduleWrapper);
	PortableExecutable(const PortableExecutable& other) = delete;
	PortableExecutable(PortableExecutable&& other) = delete;
	PortableExecutable& operator=(const PortableExecutable& other) = delete;
	PortableExecutable& operator=(PortableExecutable&& other) = delete;
	~PortableExecutable();
private:
	void CreateDOSHeader();
	void CreateFileHeader();
	void CreateOptionalHeader();
	void CreateSectionsHeaders();
	void CreateTextSection();
	void CreateStrings();
	bool CheckBounds(size_t size) const;
	bool IsImageFile() const;
	IMAGE_DOS_HEADER* _dosHeader;
	IMAGE_FILE_HEADER* _fileHeader;
	IMAGE_OPTIONAL_HEADER* _optionalHeader;
	std::vector<BYTE> _textSection;
	std::vector<StringWrapper> _strings;
	std::vector<IMAGE_SECTION_HEADER*> _sectionsHeaders;
	BYTE* _moduleBaseAddress;
	std::vector<BYTE> _buffer;
	std::vector<BYTE>::iterator _bufferIt;
};
