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
