#pragma once

#include <ICodeBuilder.h>
#include <PortableExecutable.h>
#include <Trampoline.h>

namespace Builders
{
	class CodeBuilder : Interfaces::ICodeBuilder
	{
	public:
		explicit CodeBuilder(const Models::PortableExecutable& portableExecutable);
		~CodeBuilder() override;
		Models::Trampoline CreateTrampoline(std::uintptr_t targetFunctionAddress) override;
	private:
		std::uint32_t FindCodeCave(int size) const;
		const Models::PortableExecutable& _portableExecutable;
	};
}