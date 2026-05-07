#pragma once

#include <IProcessManager.h>
#include <Module.h>

namespace Models
{
	/// <summary>
	/// Represents a Windows Portable Executable (PE) file, providing access to its headers and sections.
	/// Underlying memory is owned by ModuleWrapper, everything else is just view of that memory.
	/// </summary>
	class PortableExecutable
	{
	public:
		explicit PortableExecutable(Models::Module moduleWrapper);
		PortableExecutable(const PortableExecutable& other) = delete;
		PortableExecutable& operator=(const PortableExecutable& other) = delete;
		std::span<const IMAGE_RUNTIME_FUNCTION_ENTRY> GetAllImageRuntimeFunctionEntries() const;
		std::span<const std::byte> GetBuffer() const noexcept { return _moduleWrapper.Data(); };
		std::uintptr_t GetBaseAddress() const noexcept { return _moduleWrapper.BaseAddress(); };
		std::span<std::byte> GetTextSection() const noexcept { return _textSection; };
		const IMAGE_SECTION_HEADER* GetFileImageSectionHeader() const;
		const IMAGE_OPTIONAL_HEADER64* GetOptionalHeader() const { return _imageOptionalHeader64; };
		std::uint32_t GetExportedFunction(std::string functionName);
	private:
		IMAGE_DOS_HEADER* CreateDosHeader() const;
		IMAGE_FILE_HEADER* CreateFileHeader() const;
		IMAGE_OPTIONAL_HEADER64* CreateOptionalHeader() const;
		std::vector<IMAGE_SECTION_HEADER*> CreateSectionsHeaders() const;
		std::span<std::byte> CreateSection(const char* sectionName) const;
		bool IsImageFile() const;
		IMAGE_SECTION_HEADER* FindSectionHeader(const char* name) const;
		Models::Module _moduleWrapper;
		std::span<std::byte> _buffer;
		IMAGE_DOS_HEADER* _imageDosHeader;
		IMAGE_FILE_HEADER* _imageFileHeader;
		IMAGE_OPTIONAL_HEADER64* _imageOptionalHeader64;
		std::span<std::byte> _textSection;
		std::span<std::byte> _pDataSection;
		std::vector<IMAGE_SECTION_HEADER*> _imageSectionsHeaders;
	};
}
