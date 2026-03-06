#include <pch.h>

#include <stdexcept>
#include <format>
#include <WindowsException.h>
#include <ModuleManager.h>

namespace Managers
{
	ModuleManager::ModuleManager(const std::wstring& moduleName)
	{
		HMODULE moduleHandle = GetModuleHandle(moduleName.data());
		if (moduleHandle == nullptr)
		{
			throw Exceptions::WindowsException(L"Unable to get '{0}' module.", moduleName);
		}

		_moduleName = moduleName;
		_moduleHandle = moduleHandle;
		_moduleBaseAddress = reinterpret_cast<std::uintptr_t>(moduleHandle);
	}

	ModuleManager::~ModuleManager()
	{
	}

	FARPROC ModuleManager::GetFunctionAddress(const std::string& functionName) const
	{
		FARPROC functionAddress = GetProcAddress(_moduleHandle, functionName.data());
		if (functionAddress == nullptr)
		{
			throw Exceptions::WindowsException(L"Unable to get address of '{0}' function from '{1}' module.", std::wstring(functionName.begin(), functionName.end()), _moduleName);
		}

		return functionAddress;
	}
}
