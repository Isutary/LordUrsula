#include <windows.h>
#include <TlHelp32.h>
#include <stdexcept>
#include <iostream>

#include <ProcessManager.h>
#include <PortableExecutable.h>

ProcessManager::ProcessManager(const std::wstring& processName) : _processName(processName)
{
	_processId = ReadProcessId(processName);
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, _processId);
	if (processHandle == NULL)
	{
		throw std::runtime_error("Unable to get process handle.");
	}

	_processHandle = processHandle;

	_processModules = ReadProcessModules();

	_baseModuleMemory = ReadBaseModuleMemory();

	_portableExecutable = std::make_unique<PortableExecutable>(std::span<std::byte>(_baseModuleMemory.begin(), _baseModuleMemory.end()));
}

ProcessManager::~ProcessManager()
{
	CloseHandle(_processHandle);
	_processHandle = NULL;
}

DWORD ProcessManager::ReadProcessId(const std::wstring& processName) const
{
	HANDLE processesSnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (processesSnapshotHandle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create processes snapshot.");
	}

	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	DWORD result = 0;
	if (!Process32First(processesSnapshotHandle, &processEntry))
	{
		CloseHandle(processesSnapshotHandle);
		throw std::runtime_error("Unable to find first process in snapshot.");
	}

	do
	{
		if (std::wstring(processEntry.szExeFile) == processName)
		{
			result = processEntry.th32ProcessID;
			break;
		}
	} while (Process32Next(processesSnapshotHandle, &processEntry));

	CloseHandle(processesSnapshotHandle);

	if (result == 0)
	{
		throw std::runtime_error("Unable to find process.");
	}

	return result;
}

std::vector<MODULEENTRY32> ProcessManager::ReadProcessModules() const
{
	HANDLE modulesSnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, _processId);
	if (modulesSnapshotHandle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create modules snapshot.");
	}

	MODULEENTRY32 moduleEntry;
	moduleEntry.dwSize = sizeof(MODULEENTRY32);

	if (!Module32First(modulesSnapshotHandle, &moduleEntry))
	{
		CloseHandle(modulesSnapshotHandle);
		throw std::runtime_error("Unable to find first module in snapshot.");
	}

	std::vector<MODULEENTRY32> processModules;
	do
	{
		processModules.push_back(moduleEntry);
	} while (Module32Next(modulesSnapshotHandle, &moduleEntry));

	CloseHandle(modulesSnapshotHandle);

	return processModules;
}

std::vector<std::byte> ProcessManager::ReadBaseModuleMemory() const
{
	if (_processModules.empty())
	{
		throw std::runtime_error("No modules loaded.");
	}

	const MODULEENTRY32& baseModuleEntry = _processModules[0];

	std::vector<std::byte> buffer(baseModuleEntry.modBaseSize);
	SIZE_T bufferSize;
	if (!ReadProcessMemory(_processHandle, baseModuleEntry.modBaseAddr, buffer.data(), baseModuleEntry.modBaseSize, &bufferSize))
	{
		throw std::runtime_error("Unable to read base module memory.");
	}

	if (baseModuleEntry.modBaseSize != bufferSize)
	{
		throw std::runtime_error("Unable to read full memory of base module.");
	}

	return buffer;
}
