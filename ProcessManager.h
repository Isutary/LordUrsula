#pragma once

#include <string>
#include <windows.h>
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
	const std::wstring _processName;
	HANDLE _processHandle;
	DWORD _processId;
};
