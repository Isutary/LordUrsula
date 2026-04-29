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

	Models::Module ProcessManager::ReadModuleMemory(const std::wstring& moduleName) const
	{
		std::vector<MODULEENTRY32> processModules = Helpers::ReadSnapshotEntries<MODULEENTRY32>(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, static_cast<unsigned long>(_processId));
		if (processModules.empty())
		{
			throw Exceptions::WindowsException(L"{0} - No modules loaded.", GetProcessIdentifier());
		}

		std::vector<MODULEENTRY32>::iterator moduleEntry = std::find_if(processModules.begin(), processModules.end(), [&moduleName](const MODULEENTRY32& m)
			{
				return std::wstring(m.szExePath).find(moduleName) != std::wstring::npos;
			});

		if (moduleEntry == processModules.end())
		{
			throw Exceptions::Exception(L"{0} - Unable to find '{1}' module.", GetProcessIdentifier(), moduleName);
		}

		std::vector<std::byte> buffer(moduleEntry->modBaseSize);
		ReadMemory(reinterpret_cast<std::uintptr_t > (moduleEntry->modBaseAddr), buffer);
	
		return Models::Module(std::move(buffer), reinterpret_cast<std::uintptr_t>(moduleEntry->modBaseAddr));
	}

	std::vector<std::wstring> ProcessManager::GetAllModulesNames() const
	{
		std::vector<MODULEENTRY32> processModules = Helpers::ReadSnapshotEntries<MODULEENTRY32>(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, static_cast<unsigned long>(_processId));
		if (processModules.empty())
		{
			throw Exceptions::WindowsException(L"{0} - No modules loaded.", GetProcessIdentifier());
		}

		std::vector<std::wstring> modulesNames;

		for (std::size_t i = 0; i < processModules.size(); i++)
		{
			modulesNames.push_back(std::wstring(processModules[i].szExePath));
		}

		return modulesNames;
	}

	void ProcessManager::WriteMemory(std::uintptr_t baseAddress, std::span<const std::byte> buffer) const
	{
		std::size_t numberOfBytesWritten;
		if (!WriteProcessMemory(_processHandle, reinterpret_cast<void*>(baseAddress), buffer.data(), buffer.size(), &numberOfBytesWritten))
		{
			throw Exceptions::WindowsException(L"{0} - Unable to write bytes.", GetProcessIdentifier());
		}

		if (buffer.size() != numberOfBytesWritten)
		{
			throw Exceptions::Exception(L"{0} - Unable to write all bytes. Wrote {1} out of {2}", GetProcessIdentifier(), numberOfBytesWritten, buffer.size());
		}
	}

	void ProcessManager::ReadMemory(std::uintptr_t baseAddress, std::span<std::byte> buffer) const
	{
		std::size_t numberOfBytesRead;
		if (!ReadProcessMemory(_processHandle, reinterpret_cast<void*>(baseAddress), buffer.data(), buffer.size(), &numberOfBytesRead))
		{
			throw Exceptions::WindowsException(L"{0} - Unable to read process memory.", GetProcessIdentifier());
		}

		if (buffer.size() != numberOfBytesRead)
		{
			throw Exceptions::Exception(L"{0} - Unable to read all bytes. Read {1} out of {2}", GetProcessIdentifier(), numberOfBytesRead, buffer.size());
		}
	}

	/// <summary>
	/// Allocates virtual memory in the target process.
	/// </summary>
	/// <param name="size">The size in bytes of the memory to allocate.</param>
	/// <returns>The base address of the allocated memory as an unsigned integer pointer. Throws if unable to allocate memory.</returns>
	std::uintptr_t ProcessManager::AllocateVirtualMemory(std::size_t size) const
	{
		void* allocatedMemoryBase = VirtualAllocEx(_processHandle, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (allocatedMemoryBase == nullptr)
		{
			throw Exceptions::WindowsException(L"{0} - Unable to allocate virtual memory. Attempted to allocate {0} bytes.", GetProcessIdentifier(), size);
		}

		return reinterpret_cast<std::uintptr_t>(allocatedMemoryBase);
	}

	void ProcessManager::LoadRemoteLibrary(const std::wstring& libraryPath, const Interfaces::IModuleManager& moduleManager) const
	{
		// Convert library path to bytes.
		std::span<const std::byte> buffer = std::as_bytes(std::span{ libraryPath.data(), libraryPath.size() + 1 });

		// Allocate enough memory in process to store path to library.
		std::uintptr_t allocatedMemoryBase = AllocateVirtualMemory(buffer.size());

		// Write library path in newly allocate memory of the process.
		WriteMemory(allocatedMemoryBase, buffer);

		// Find address of LoadLibraryW function in specified module.
		FARPROC loadLibraryWAddress = moduleManager.GetFunctionAddress("LoadLibraryW");

		// Create remote thread in the process to call LoadLibraryW with path to library to load.
		std::uintptr_t remoteThreadHandle = CreateRemoteThread(loadLibraryWAddress, allocatedMemoryBase);

		// Wait for thread to finish execution.
		WaitForThread(remoteThreadHandle);
	}

	/// <summary>
	/// Creates a remote thread in the target process.
	/// </summary>
	/// <param name="functionBaseAddress">The base address of the function to execute in the remote thread.</param>
	/// <param name="parameterAddress">The address of the parameter to pass to the remote thread function.</param>
	/// <returns>The handle to the created remote thread as an unsigned integer pointer. Throws if unable to create remote thread.</returns>
	std::uintptr_t ProcessManager::CreateRemoteThread(FARPROC functionBaseAddress, std::uintptr_t parameterAddress) const
	{
		void* remoteThreadHandle = CreateRemoteThreadEx(_processHandle, nullptr, 0, (LPTHREAD_START_ROUTINE)functionBaseAddress, reinterpret_cast<void*>(parameterAddress), 0, nullptr, nullptr);
		if (remoteThreadHandle == nullptr)
		{
			throw Exceptions::WindowsException(L"{0} - Unable to create remote thread.", GetProcessIdentifier());
		}

		return reinterpret_cast<std::uintptr_t>(remoteThreadHandle);
	}

	std::size_t ProcessManager::WaitForThread(std::uintptr_t remoteThreadHandle) const
	{
		return WaitForSingleObject(reinterpret_cast<void*>(remoteThreadHandle), INFINITE);
	}

	std::size_t ProcessManager::ReadProcessId(const std::wstring& processName) const
	{
		std::vector<PROCESSENTRY32> processEntries = Helpers::ReadSnapshotEntries<PROCESSENTRY32>(TH32CS_SNAPPROCESS);

		std::vector<PROCESSENTRY32>::iterator processEntry = std::find_if(processEntries.begin(), processEntries.end(), [&processName](const PROCESSENTRY32& p)
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
