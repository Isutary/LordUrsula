#include <pch.h>

#include <PortableExecutable.h>
#include <Module.h>
#include <CoutHelper.h>

namespace Models
{
	PortableExecutable::PortableExecutable(Models::Module moduleWrapper) : _moduleWrapper(std::move(moduleWrapper)), _buffer(_moduleWrapper.Data())
	{
		if (!IsImageFile())
		{
			throw std::runtime_error("File is not a PE.");
		}

		_imageDosHeader = CreateDosHeader();
		_imageFileHeader = CreateFileHeader();
		_imageOptionalHeader64 = CreateOptionalHeader();
		_imageSectionsHeaders = CreateSectionsHeaders();
		_textSection = CreateSection(".text");
		_pDataSection = CreateSection(".pdata");
	}

	std::span<const IMAGE_RUNTIME_FUNCTION_ENTRY> PortableExecutable::GetAllImageRuntimeFunctionEntries() const
	{
		// Calculate number of IMAGE_RUNTIME_FUNCTION_ENTRY.
		std::size_t numberOfImageRuntimeFunctionEntries = _pDataSection.size() / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);

		// Reinterpret raw bytes as IMAGE_RUNTIME_FUNCTION_ENTRY.
		const IMAGE_RUNTIME_FUNCTION_ENTRY* data = reinterpret_cast<const IMAGE_RUNTIME_FUNCTION_ENTRY*>(_pDataSection.data());

		// Create span of all IMAGE_RUNTIME_FUNCTION_ENTRY.
		return std::span<const IMAGE_RUNTIME_FUNCTION_ENTRY>(data, numberOfImageRuntimeFunctionEntries);
	}

	const IMAGE_SECTION_HEADER* PortableExecutable::GetFileImageSectionHeader() const
	{
		std::vector<IMAGE_SECTION_HEADER*>::const_iterator textSectionHeaderIt = std::find_if(_imageSectionsHeaders.begin(), _imageSectionsHeaders.end(), [](const IMAGE_SECTION_HEADER* currentImageSectionHeader)
			{
				char nameDestination[9] = {};
				std::memcpy(nameDestination, currentImageSectionHeader->Name, sizeof(currentImageSectionHeader->Name));
				return std::strcmp(nameDestination, ".text") == 0;
			});

		if (textSectionHeaderIt == _imageSectionsHeaders.end())
		{
			throw std::runtime_error("Unable to find .text section header.");
		}

		return *textSectionHeaderIt;
	}

	std::uint32_t PortableExecutable::GetExportedFunction(std::string targetFunctionName)
	{
		// First entry in IMAGE_OPTIONAL_HEADER64.DataDirectory contains address and size of export table.
		IMAGE_DATA_DIRECTORY imageExportDataDirectory = _imageOptionalHeader64->DataDirectory[0];

		// Calculate virtual address of IMAGE_EXPORT_DIRECTORY.
		std::byte* imageExportDirectoryAddress = _buffer.data() + imageExportDataDirectory.VirtualAddress;

		// Reinterpret raw bytes as IMAGE_EXPORT_DIRECTORY.
		IMAGE_EXPORT_DIRECTORY* imageExportDirectory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(imageExportDirectoryAddress);

		// Create export name pointer table.
		DWORD* exportNamePointerTable = reinterpret_cast<DWORD*>(_buffer.data() + imageExportDirectory->AddressOfNames);

		// Find index of target function in export name pointers table.
		std::size_t i = 0;
		for (i; i < imageExportDirectory->NumberOfNames; i++)
		{
			char* name = reinterpret_cast<char*>(_buffer.data() + exportNamePointerTable[i]);
			if (std::string(name) == targetFunctionName)
			{
				break;
			}
		}

		// If we reach the end of previous loop, it means we didn't find target function name.
		if (i == imageExportDirectory->NumberOfNames)
		{
			throw std::runtime_error("Unable to find specified function name.");
		}

		// Create export ordinal table.
		// Unlike name pointer table and function pointer table, the export ordinal table is an array of 16-bit unbiased indexes, 
		// so we cast it do interprete it as WORD instead.
		WORD* exportOrdinalTable = reinterpret_cast<WORD*>(_buffer.data() + imageExportDirectory->AddressOfNameOrdinals);
		
		// Because not all exported functions have name defined in name pointer table, we have to use ordinal table to get associated index in function pointer table. 
		auto o = exportOrdinalTable[i];

		// Create export function pointer table.
		DWORD* exportFunctionPointerTable = reinterpret_cast<DWORD*>(_buffer.data() + imageExportDirectory->AddressOfFunctions);
		
		return exportFunctionPointerTable[o];
	}

	IMAGE_DOS_HEADER* PortableExecutable::CreateDosHeader() const
	{
		// Size of buffer is validated in IsImageFile method.
		// Create IMAGE_DOS_HEADER.
		return reinterpret_cast<IMAGE_DOS_HEADER*>(_buffer.data());
	}

	IMAGE_FILE_HEADER* PortableExecutable::CreateFileHeader() const
	{
		// IMAGE_DOS_HEADER.e_lfanew defines the offset of the PE signature. 
		// COFF file header comes after the PE signature, so we are skipping 4 bytes of the signature.
		std::size_t imageFileHeaderOffset = static_cast<std::size_t>(_imageDosHeader->e_lfanew) + 4;

		// Validate the buffer has enough bytes to contain IMAGE_FILE_HEADER.
		if (_buffer.size() < imageFileHeaderOffset + sizeof(IMAGE_FILE_HEADER))
		{
			throw std::runtime_error("Not enough bytes for IMAGE_FILE_HEADER.");
		}

		// Create IMAGE_FILE_HEADER.
		return reinterpret_cast<IMAGE_FILE_HEADER*>(_buffer.data() + imageFileHeaderOffset);
	}

	IMAGE_OPTIONAL_HEADER64* PortableExecutable::CreateOptionalHeader() const
	{
		// IMAGE_DOS_HEADER.e_lfanew defines the offset of the PE signature.
		// COFF file header comes after the PE signature, so we are skipping 4 bytes of the signature.
		// Optional header comes immediately after image file header.
		std::size_t imageOptionalHeaderOffset = static_cast<std::size_t>(_imageDosHeader->e_lfanew) + 4 + sizeof(IMAGE_FILE_HEADER);

		// Validate the buffer has enough bytes to contain IMAGE_OPTIONAL_HEADER.
		if (_buffer.size() < imageOptionalHeaderOffset + _imageFileHeader->SizeOfOptionalHeader)
		{
			throw std::runtime_error("Not enough bytes for IMAGE_OPTIONAL_HEADER.");
		}

		// The optional header magic number determines whether an image is a PE32 or PE32+ executable.
		// The magic number is defined by first 2 bytes of the IMAGE_OPTIONAL_HEADER.
		std::uint16_t peFormat = *reinterpret_cast<std::uint16_t*>(_buffer.data() + imageOptionalHeaderOffset);

		// Only 64-bit modules are supported.
		if (peFormat != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			throw std::runtime_error("Module is not 64-bit.");
		}
			
		// Create IMAGE_OPTIONAL_HEADER64.
		return reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(_buffer.data() + imageOptionalHeaderOffset);
	}

	std::vector<IMAGE_SECTION_HEADER*> PortableExecutable::CreateSectionsHeaders() const
	{
		// IMAGE_DOS_HEADER.e_lfanew defines the offset of the PE signature.
		// COFF file header comes after the PE signature, so we are skipping 4 bytes of the signature.
		// Section headers come immediately after optional headers. 
		// Size of optional header is taken from IMAGE_FILE_HEADER.SizeOfOptionalHeader.
		std::size_t imageSectionHeadersOffset = static_cast<std::size_t>(_imageDosHeader->e_lfanew) + 4 + sizeof(IMAGE_FILE_HEADER) + _imageFileHeader->SizeOfOptionalHeader;

		// Validate the buffer has enough bytes to contain all IMAGE_SECTION_HEADERs.
		if (_buffer.size() < imageSectionHeadersOffset + _imageFileHeader->NumberOfSections * sizeof(IMAGE_SECTION_HEADER))
		{
			throw std::runtime_error("Not enough bytes for all IMAGE_SECTION_HEADERs.");
		}

		// Reserve enough space for all IMAGE_SECTION_HEADERs.
		std::vector<IMAGE_SECTION_HEADER*> imageSectionsHeaders(_imageFileHeader->NumberOfSections);

		// Create all IMAGE_SECTION_HEADERs.
		for (std::uint16_t i = 0; i < _imageFileHeader->NumberOfSections; i++)
		{
			std::size_t currentImageSectionHeaderOffset = imageSectionHeadersOffset + i * sizeof(IMAGE_SECTION_HEADER);
			imageSectionsHeaders[i] = reinterpret_cast<IMAGE_SECTION_HEADER*>(_buffer.data() + currentImageSectionHeaderOffset);
		}

		return imageSectionsHeaders;
	}

	std::span<std::byte> PortableExecutable::CreateSection(const char* sectionName) const
	{
		// Find IMAGE_SECTION_HEADER with name specified name.
		IMAGE_SECTION_HEADER* sectionHeader = FindSectionHeader(sectionName);

		// IMAGE_SECTION_HEADER.VirtualAddress defines address of the first byte of the section, relative to the image base.
		std::span<std::byte>::iterator sectionStartIt = _buffer.begin() + sectionHeader->VirtualAddress;

		// IMAGE_SECTION_HEADER.VirtualSize defines total size of the section.
		std::span<std::byte>::iterator sectionEndIt = sectionStartIt + sectionHeader->Misc.VirtualSize;

		// Calculate size of the section.
		std::ptrdiff_t sizeOfSection = sectionEndIt - sectionStartIt;

		// Create span containing the section.
		return std::span<std::byte>(sectionStartIt, sizeOfSection);
	}

	bool PortableExecutable::IsImageFile() const
	{
		// Validate the buffer has enough bytes(64) to contain the PE signature offset.
		// PE signature offset is at location 60(0x3C) plus 4 bytes of the offset itself. Simply put size of IMAGE_DOS_HEADER.
		if (_buffer.size() < sizeof(IMAGE_DOS_HEADER))
		{
			throw std::runtime_error("Not enough bytes to find PE signature offset.");
		}

		// At location 60(0x3C), the stub has the file offset to the PE signature. 
		std::uint32_t peSignatureOffset = *reinterpret_cast<std::uint32_t*>(_buffer.data() + 0x3C);

		// Validate the buffer has enough bytes to contain the PE signature.
		if (_buffer.size() < static_cast<std::size_t>(peSignatureOffset) + 4)
		{
			throw std::runtime_error("Not enough bytes to find PE signature.");
		}

		std::span<std::byte>::iterator peSignatureIt = _buffer.begin() + peSignatureOffset;

		// The PE signature is "PE\0\0" (the letters "P"(0x50) and "E"(0x45) followed by two null bytes).
		constexpr std::array peImageSignature = { std::byte(0x50), std::byte(0x45), std::byte(0x00), std::byte(0x00) };

		return std::ranges::equal(peImageSignature.begin(), peImageSignature.end(), peSignatureIt, peSignatureIt + 4);
	}

	IMAGE_SECTION_HEADER* PortableExecutable::FindSectionHeader(const char* name) const
	{
		std::vector<IMAGE_SECTION_HEADER*>::const_iterator imageSectionHeaderIt = std::find_if(_imageSectionsHeaders.begin(), _imageSectionsHeaders.end(), [name](const IMAGE_SECTION_HEADER* sectionHeader)
			{
				char nameDestination[9] = {};
				std::memcpy(nameDestination, sectionHeader->Name, sizeof(sectionHeader->Name));
				return std::strcmp(nameDestination, name) == 0;
			});

		if (imageSectionHeaderIt == _imageSectionsHeaders.end())
		{
			throw std::runtime_error("Unable to find section header.");
		}

		return *imageSectionHeaderIt;
	}
}
