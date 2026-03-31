#include "pch.h"

#include <PortableExecutable.h>
#include <ModuleWrapper.h>
#include <CoutHelper.h>

namespace Models
{
	PortableExecutable::PortableExecutable(ModuleWrapper moduleWrapper) : _moduleWrapper(std::move(moduleWrapper)), _buffer(_moduleWrapper.Data())
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

	PortableExecutable::~PortableExecutable()
	{}

	IMAGE_DOS_HEADER* PortableExecutable::CreateDosHeader() const
	{
		// Size of buffer is validated in IsImageFile method.
		// Create IMAGE_DOS_HEADER.
		return reinterpret_cast<IMAGE_DOS_HEADER*>(std::to_address(_buffer.begin()));
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
		return reinterpret_cast<IMAGE_FILE_HEADER*>(std::to_address(_buffer.begin() + imageFileHeaderOffset));
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
		std::uint16_t peFormat = *reinterpret_cast<std::uint16_t*>(std::to_address(_buffer.begin() + imageOptionalHeaderOffset));

		// Only 64-bit modules are supported.
		if (peFormat != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			throw std::runtime_error("Module is not 64-bit.");
		}
			
		// Create IMAGE_OPTIONAL_HEADER64.
		return reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(std::to_address(_buffer.begin() + imageOptionalHeaderOffset));
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
			imageSectionsHeaders[i] = reinterpret_cast<IMAGE_SECTION_HEADER*>(std::to_address(_buffer.begin() + currentImageSectionHeaderOffset));
		}

		return imageSectionsHeaders;
	}

	std::span<std::byte> PortableExecutable::CreateSection(const char* sectionName) const
	{
		// Find IMAGE_SECTION_HEADER with name specified name.
		IMAGE_SECTION_HEADER* textSectionHeader = FindSectionHeader(sectionName);

		// IMAGE_SECTION_HEADER.VirtualAddress defines address of the first byte of the section, relative to the image base.
		std::span<std::byte>::iterator textSectionStartIt = _buffer.begin() + textSectionHeader->VirtualAddress;

		// IMAGE_SECTION_HEADER.VirtualSize defines total size of the section.
		std::span<std::byte>::iterator textSectionEndIt = textSectionStartIt + textSectionHeader->Misc.VirtualSize;

		// Calculate size of the section.
		std::ptrdiff_t sizeOfTextSection = textSectionEndIt - textSectionStartIt;

		// Create span containing the section.
		return std::span<std::byte>(textSectionStartIt, sizeOfTextSection);
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
		std::uint32_t peSignatureOffset = *reinterpret_cast<std::uint32_t*>(std::to_address(_buffer.begin() + 0x3C));

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
