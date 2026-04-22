#pragma once

#include <ICodeBuilder.h>
#include <PortableExecutable.h>

namespace Builders
{
	class CodeBuilder : Interfaces::ICodeBuilder
	{
	public:
		explicit CodeBuilder(const Models::PortableExecutable& portableExecutable);
		~CodeBuilder() override;
		void CreateTrampoline() override;
	private:
		const Models::PortableExecutable& _portableExecutable;
	};
}