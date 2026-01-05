#pragma once

#include <windows.h>
#include <vector>
#include <span>

class PortableExecutable
{
public:
	PortableExecutable(std::vector<std::byte> buffer);
	~PortableExecutable();
private:
	void CreateDOSHeader();
	void CreateFileHeader();
	void CreateOptionalHeader();
	void CreateSectionsHeaders();
	void CreateTextSection();
	bool CheckBounds(size_t size) const;
	bool IsImageFile() const;
	IMAGE_DOS_HEADER* _dosHeader;
	IMAGE_FILE_HEADER* _fileHeader;
	IMAGE_OPTIONAL_HEADER* _optionalHeader;
	std::vector<std::byte*> _textSection;
	std::vector<IMAGE_SECTION_HEADER*> _sectionsHeaders;
	std::vector<std::byte> _buffer;
	std::vector<std::byte>::iterator _bufferIt;
};
