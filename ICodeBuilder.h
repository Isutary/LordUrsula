#pragma once

namespace Builders::Interfaces
{
	class ICodeBuilder
	{
	public:
		virtual ~ICodeBuilder() = default;
		virtual void CreateTrampoline() = 0;
	};
}
