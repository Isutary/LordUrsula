#pragma once

#include "pch.h"

#include <ModuleWrapper.h>

class ProcessManager
{
public:
	ProcessManager(const std::wstring& processName);
	ProcessManager(const ProcessManager& other) = delete;
	ProcessManager(ProcessManager&& other) = delete;
	ProcessManager& operator=(const ProcessManager& other) = delete;
	ProcessManager& operator=(ProcessManager&& other) = delete;
	~ProcessManager();
	const std::wstring& GetProcessName() const { return _processName; }
	ModuleWrapper ReadBaseModuleMemory() const;
	void LoadTargetLibrary(const std::wstring& targetPath) const;
private:
	DWORD ReadProcessId(const std::wstring& processName) const;
	const std::wstring _processName;
	HANDLE _processHandle;
	DWORD _processId;
};
