#include <pch.h>

#include <SnapshotHelper.h>
#include <ProcessManager.h>
#include <IModuleManager.h>
#include <Exception.h>
#include <WindowsException.h>

namespace Managers
{
	ProcessManager::ProcessManager(const std::wstring& processName) : _processName(processName)
	{
		_processId = ReadProcessId(processName);
		void* processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, static_cast<unsigned long>(_processId));
		if (processHandle == nullptr)
		{
			throw Exceptions::WindowsException(L"Process: {0} - Unable to get process handle.", processName);
		}

		_processHandle = processHandle;
	}

	ProcessManager::~ProcessManager()
	{
		if (_processHandle != nullptr)
		{
			CloseHandle(_processHandle);
			_processHandle = nullptr;
		}
	}

	ModuleWrapper ProcessManager::ReadModuleMemory(const std::wstring& moduleName) const
	{
		std::vector<MODULEENTRY32> processModules = ReadSnapshotEntries<MODULEENTRY32>(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, static_cast<unsigned long>(_processId));
		if (processModules.empty())
		{
			throw Exceptions::WindowsException(L"{0}No modules loaded.", GetProcessIdentifier());
		}

		auto moduleEntry = std::find_if(processModules.begin(), processModules.end(), [&moduleName](const MODULEENTRY32& m)
			{
				return std::wstring(m.szExePath) == moduleName;
			});

		if (moduleEntry == processModules.end())
		{
			throw Exceptions::Exception(L"{0}Unable to find '{1}' module.", GetProcessIdentifier(), moduleName);
		}

		std::vector<std::byte> buffer(moduleEntry->modBaseSize);
		ReadMemory(reinterpret_cast<std::uintptr_t > (moduleEntry->modBaseAddr), buffer);
		
		return ModuleWrapper(std::move(buffer), reinterpret_cast<std::uintptr_t>(moduleEntry->modBaseAddr));
	}

	void ProcessManager::WriteMemory(std::uintptr_t baseAddress, std::span<const std::byte> buffer) const
	{
		std::size_t numberOfBytesWritten;
		if (!WriteProcessMemory(_processHandle, reinterpret_cast<void*>(baseAddress), buffer.data(), buffer.size(), &numberOfBytesWritten))
		{
			throw Exceptions::WindowsException(L"{0}Unable to write bytes.", GetProcessIdentifier());
		}

		if (buffer.size() != numberOfBytesWritten)
		{
			throw Exceptions::Exception(L"{0}Unable to write all bytes. Wrote {1} out of {2}", GetProcessIdentifier(), numberOfBytesWritten, buffer.size());
		}
	}

	void ProcessManager::ReadMemory(std::uintptr_t baseAddress, std::span<std::byte> buffer) const
	{
		std::size_t numberOfBytesRead;
		if (!ReadProcessMemory(_processHandle, reinterpret_cast<void*>(baseAddress), buffer.data(), buffer.size(), &numberOfBytesRead))
		{
			throw Exceptions::WindowsException(L"{0}Unable to read process memory.", GetProcessIdentifier());
		}

		if (buffer.size() != numberOfBytesRead)
		{
			throw Exceptions::Exception(L"{0}Unable to read all bytes. Read {1} out of {2}", GetProcessIdentifier(), numberOfBytesRead, buffer.size());
		}
	}

	std::uintptr_t ProcessManager::AllocateVirtualMemory(std::size_t size) const
	{
		void* allocatedMemoryBase = VirtualAllocEx(_processHandle, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (allocatedMemoryBase == nullptr)
		{
			throw Exceptions::WindowsException(L"{0}Unable to allocate virtual memory. Attempted to allocate {0} bytes.", GetProcessIdentifier(), size);
		}

		return reinterpret_cast<std::uintptr_t>(allocatedMemoryBase);
	}

	void ProcessManager::LoadRemoteLibrary(const std::wstring& libraryPath, const Interfaces::IModuleManager& moduleManager) const
	{
		std::span<const std::byte> buffer = std::as_bytes(std::span{ libraryPath.data(), libraryPath.size() + 1 });
		std::uintptr_t allocatedMemoryBase = AllocateVirtualMemory(buffer.size());

		WriteMemory(allocatedMemoryBase, buffer);

		FARPROC loadLibraryWAddress = moduleManager.GetFunctionAddress("LoadLibraryW");

		std::uintptr_t remoteThreadHandle = CreateRemoteThread(loadLibraryWAddress, allocatedMemoryBase);

		WaitForThread(remoteThreadHandle);
	}

	std::uintptr_t ProcessManager::CreateRemoteThread(FARPROC functionBaseAddress, std::uintptr_t parameterAddress) const
	{
		void* remoteThreadHandle = CreateRemoteThreadEx(_processHandle, nullptr, 0, (LPTHREAD_START_ROUTINE)functionBaseAddress, reinterpret_cast<void*>(parameterAddress), 0, nullptr, nullptr);
		if (remoteThreadHandle == nullptr)
		{
			throw Exceptions::WindowsException(L"{0}Unable to create remote thread.", GetProcessIdentifier());
		}

		return reinterpret_cast<std::uintptr_t>(remoteThreadHandle);
	}

	std::size_t ProcessManager::WaitForThread(std::uintptr_t remoteThreadHandle) const
	{
		return WaitForSingleObject(reinterpret_cast<void*>(remoteThreadHandle), INFINITE);
	}

	std::size_t ProcessManager::ReadProcessId(const std::wstring& processName) const
	{
		std::vector<PROCESSENTRY32> processEntries = ReadSnapshotEntries<PROCESSENTRY32>(TH32CS_SNAPPROCESS);

		auto processEntry = std::find_if(processEntries.begin(), processEntries.end(), [&processName](const PROCESSENTRY32& p)
			{
				return std::wstring(p.szExeFile) == processName;
			});

		if (processEntry == processEntries.end())
		{
			throw Exceptions::WindowsException(L"Unable to find '{0}' process.", processName);
		}

		return processEntry->th32ProcessID;
	}

	std::wstring ProcessManager::GetProcessIdentifier() const
	{
		return std::format(L"Process: {0}({1})", _processName, _processId);
	}
}
