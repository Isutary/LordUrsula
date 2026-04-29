#pragma once

#include <Trampoline.h>

namespace Builders::Interfaces
{
	class ICodeBuilder
	{
	public:
		virtual ~ICodeBuilder() = default;
		virtual Models::Trampoline CreateTrampoline(std::uintptr_t targetFunction) = 0;
	};
}
