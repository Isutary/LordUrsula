#pragma once

#include <IModuleManager.h>

namespace Managers
{
	class ModuleManager : public Interfaces::IModuleManager
	{
	public:
		ModuleManager(const std::wstring& moduleName);
		~ModuleManager() override;
		FARPROC GetFunctionAddress(const std::string& functionName) const override;
		std::uintptr_t GetBaseAddress() const override { return _moduleBaseAddress; };
		const std::wstring& GetModuleName() const override { return _moduleName; };
	private:
		std::wstring _moduleName;
		HMODULE _moduleHandle;
		std::uintptr_t _moduleBaseAddress;
	};
}
