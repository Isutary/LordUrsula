#include "pch.h"

#include <ProcessManager.h>
#include <SnapshotHelper.h>
#include <ModuleWrapper.h>

ProcessManager::ProcessManager(const std::wstring& processName) : _processName(processName)
{
	_processId = ReadProcessId(processName);
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, _processId);
	if (processHandle == NULL)
	{
		throw std::runtime_error("Unable to get process handle.");
	}

	_processHandle = processHandle;
}

ProcessManager::~ProcessManager()
{
	CloseHandle(_processHandle);
	_processHandle = NULL;
}


ModuleWrapper ProcessManager::ReadBaseModuleMemory() const
{
	std::vector<MODULEENTRY32> processModules = ReadSnapshotEntries<MODULEENTRY32>(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, _processId);
	if (processModules.empty())
	{
		throw std::runtime_error("No modules loaded.");
	}

	const MODULEENTRY32& baseModuleEntry = processModules[0];

	std::vector<BYTE> buffer(baseModuleEntry.modBaseSize);
	SIZE_T bufferSize;
	if (!ReadProcessMemory(_processHandle, baseModuleEntry.modBaseAddr, buffer.data(), baseModuleEntry.modBaseSize, &bufferSize))
	{
		throw std::runtime_error("Unable to read base module memory.");
	}

	if (baseModuleEntry.modBaseSize != bufferSize)
	{
		throw std::runtime_error("Unable to read full memory of base module.");
	}

	return ModuleWrapper{ buffer, baseModuleEntry.modBaseAddr };
}

void ProcessManager::LoadTargetLibrary(const std::wstring& targetPath) const
{
	size_t sizeInBytes = (targetPath.length() + 1) * sizeof(wchar_t);
	LPVOID allocatedMemoryAddress = VirtualAllocEx(_processHandle, NULL, sizeInBytes, MEM_COMMIT, PAGE_READWRITE);
	if (allocatedMemoryAddress == NULL)
	{
		throw std::runtime_error("Unable to allocate virtual memory.");
	}

	SIZE_T bytesWritten = 0;
	if (!WriteProcessMemory(_processHandle, allocatedMemoryAddress, targetPath.data(), sizeInBytes, &bytesWritten))
	{
		throw std::runtime_error("Unable to write process memory.");
	}

	if (bytesWritten != sizeInBytes)
	{
		throw std::runtime_error("Unable to write entire buffer");
	}
	
	HMODULE kernel32ModuleHandle = GetModuleHandle(L"kernel32.dll");
	if (kernel32ModuleHandle == NULL)
	{
		throw std::runtime_error("Unable to get kernel32.dll handle.");
	}

	FARPROC loadLibraryWAddress = GetProcAddress(kernel32ModuleHandle, "LoadLibraryW");
	if (loadLibraryWAddress == NULL)
	{
		throw std::runtime_error("Unable to get address of LoadLibrary function.");
	}

	HANDLE remoteThreadHandle = CreateRemoteThreadEx(_processHandle, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryWAddress, allocatedMemoryAddress, 0, NULL, NULL);

	if (remoteThreadHandle == NULL)
	{
		throw std::runtime_error("Unable to create remote thread.");
	}

	WaitForSingleObject(remoteThreadHandle, INFINITE);
}

DWORD ProcessManager::ReadProcessId(const std::wstring& processName) const
{
	std::vector<PROCESSENTRY32> processEntries = ReadSnapshotEntries<PROCESSENTRY32>(TH32CS_SNAPPROCESS);

	auto processEntry = std::find_if(processEntries.begin(), processEntries.end(), [processName](const PROCESSENTRY32& e)
		{
			if (std::wstring(e.szExeFile) == processName) return true;
			else return false;
		});

	if (processEntry == processEntries.end())
	{
		throw std::runtime_error("Unable to find process with specified name.");
	}

	return processEntry->th32ProcessID;
}
