#pragma once

#include "pch.h"

#include <IProcessManager.h>
#include <ModuleWrapper.h>

namespace Models
{
	class PortableExecutable
	{
	public:
		PortableExecutable(ModuleWrapper moduleWrapper);
		~PortableExecutable();
	private:
		IMAGE_DOS_HEADER* CreateDosHeader() const;
		IMAGE_FILE_HEADER* CreateFileHeader() const;
		IMAGE_OPTIONAL_HEADER64* CreateOptionalHeader() const;
		std::vector<IMAGE_SECTION_HEADER*> CreateSectionsHeaders() const;
		std::span<std::byte> CreateSection(const char* sectionName) const;
		bool IsImageFile() const;
		IMAGE_SECTION_HEADER* FindSectionHeader(const char* name) const;
		IMAGE_DOS_HEADER* _imageDosHeader;
		IMAGE_FILE_HEADER* _imageFileHeader;
		IMAGE_OPTIONAL_HEADER64* _imageOptionalHeader64;
		std::span<std::byte> _textSection;
		std::span<std::byte> _pDataSection;
		std::vector<IMAGE_SECTION_HEADER*> _imageSectionsHeaders;
		ModuleWrapper _moduleWrapper;
		std::span<std::byte> _buffer;
	};
}
