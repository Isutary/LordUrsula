#pragma once

#include <string>
#include <memory>
#include <windows.h>
#include <TlHelp32.h>

#include <PortableExecutable.h>

class ProcessManager
{
public:
	ProcessManager(const std::wstring& processName);
	~ProcessManager();
	const std::wstring& GetProcessName() const { return _processName; }
private:
	DWORD ReadProcessId(const std::wstring& processName) const;
	std::vector<MODULEENTRY32> ReadProcessModules() const;
	std::vector<std::byte> ReadBaseModuleMemory() const;
	const std::wstring _processName;
	HANDLE _processHandle;
	DWORD _processId;
	std::vector<MODULEENTRY32> _processModules;
	std::vector<std::byte> _baseModuleMemory;
	std::unique_ptr<PortableExecutable> _portableExecutable;
};
