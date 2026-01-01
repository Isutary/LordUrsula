#pragma once

#include <string>
#include <memory>
#include <windows.h>
#include <TlHelp32.h>
#include <vector>

class ProcessManager
{
public:
	ProcessManager(const std::wstring& processName);
	~ProcessManager();
	const std::wstring& GetProcessName() const { return _processName; }
	std::vector<std::byte> ReadBaseModuleMemory() const;
private:
	DWORD ReadProcessId(const std::wstring& processName) const;
	std::vector<MODULEENTRY32> ReadProcessModules() const;
	const std::wstring _processName;
	HANDLE _processHandle;
	DWORD _processId;
	std::vector<MODULEENTRY32> _processModules;
};
