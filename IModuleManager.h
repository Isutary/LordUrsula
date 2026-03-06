#pragma once

#include <Windows.h>
#include <string>

namespace Managers::Interfaces
{
	class IModuleManager
	{
	public:
		virtual ~IModuleManager() = default;
		virtual FARPROC GetFunctionAddress(const std::string& functionName) const = 0;
		virtual std::uintptr_t GetBaseAddress() const = 0;
		virtual const std::wstring& GetModuleName() const = 0;
	};
}
