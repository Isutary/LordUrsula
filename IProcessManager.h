#pragma once

#include <vector>
#include <span>
#include <string>
#include <ModuleWrapper.h>
#include <IModuleManager.h>

namespace Managers::Interfaces
{
	class IProcessManager
	{
	public:
		virtual ~IProcessManager() = default;
		virtual ModuleWrapper ReadModuleMemory(const std::wstring& moduleName) const = 0;
		virtual void WriteMemory(std::uintptr_t baseAddress, std::span<const std::byte> buffer) const = 0;
		virtual void ReadMemory(std::uintptr_t baseAddress, std::span<std::byte> buffer) const = 0;
		virtual std::uintptr_t AllocateVirtualMemory(std::size_t size) const = 0;
		virtual void LoadRemoteLibrary(const std::wstring& libraryPath, const IModuleManager& moduleManager) const = 0;
		virtual std::uintptr_t CreateRemoteThread(FARPROC functionBaseAddress, std::uintptr_t parameterAddress) const = 0;
		virtual std::size_t WaitForThread(std::uintptr_t remoteThreadHandle) const = 0;
	};
}
