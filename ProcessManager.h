#pragma once

#include <vector>
#include <span>

#include <IProcessManager.h>
#include <ModuleWrapper.h>

namespace Managers
{
	class ProcessManager : public Interfaces::IProcessManager
	{
	public:
		ProcessManager(const std::wstring& processName);
		~ProcessManager() override;
		ModuleWrapper ReadModuleMemory(const std::wstring& moduleName) const override;
		void WriteMemory(std::uintptr_t baseAddress, std::span<const std::byte> buffer) const override;
		void ReadMemory(std::uintptr_t baseAddress, std::span<std::byte> buffer) const override;
		std::uintptr_t AllocateVirtualMemory(std::size_t size) const override;
		void LoadRemoteLibrary(const std::wstring& libraryPath, const Interfaces::IModuleManager& moduleManager) const override;
		std::uintptr_t CreateRemoteThread(FARPROC functionBaseAddress, std::uintptr_t parameterAddress) const override;
		std::size_t WaitForThread(std::uintptr_t remoteThreadHandle) const override;
	private:
		std::size_t ReadProcessId(const std::wstring& processName) const;
		std::wstring GetProcessIdentifier() const;
		std::size_t _processId;
		std::wstring _processName;
		void* _processHandle;
	};
}