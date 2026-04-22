#include <pch.h>

#include <CodeBuilder.h>

namespace Builders
{
	CodeBuilder::CodeBuilder(const Models::PortableExecutable& portableExecutable) : _portableExecutable(portableExecutable)
	{
		
	}

	CodeBuilder::~CodeBuilder()
	{
	}

	void CodeBuilder::CreateTrampoline()
	{

	}
}