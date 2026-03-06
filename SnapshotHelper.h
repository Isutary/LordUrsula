#pragma once

#include <windows.h>
#include <TlHelp32.h>
#include <WindowsException.h>

template<typename T>
concept SnapshotEntry = std::is_same_v<T, PROCESSENTRY32> || std::is_same_v<T, MODULEENTRY32>;

template<typename T>
bool First(HANDLE handle, T& entry);

template<typename T>
bool Next(HANDLE handle, T& entry);

template <SnapshotEntry T>
std::vector<T> ReadSnapshotEntries(DWORD flags, DWORD processId = 0)
{
	HANDLE handle = CreateToolhelp32Snapshot(flags, processId);
	if (handle == INVALID_HANDLE_VALUE)
	{
		throw Exceptions::WindowsException(L"Failed to create snapshot for process with ID '{0}' and flags '{1}'.", processId, flags);
	}

	T entry;
	entry.dwSize = sizeof(entry);

	if (!First(handle, entry))
	{
		CloseHandle(handle);
		throw Exceptions::WindowsException(L"Failed to find first entry in snapshot for process with ID '{0}' and flags '{1}'.", processId, flags);
	}

	std::vector<T> entries;
	do
	{
		entries.push_back(entry);
	} while (Next(handle, entry));

	CloseHandle(handle);
	return entries;
}

template <SnapshotEntry T>
bool First(HANDLE handle, T& entry)
{
	if constexpr (std::is_same_v<T, PROCESSENTRY32>)
	{
		return Process32First(handle, &entry);
	}
	
	if constexpr (std::is_same_v<T, MODULEENTRY32>)
	{
		return Module32First(handle, &entry);
	}
}

template <SnapshotEntry T>
bool Next(HANDLE handle, T& entry)
{
	if constexpr (std::is_same_v<T, PROCESSENTRY32>)
	{
		return Process32Next(handle, &entry);
	}
	
	if constexpr (std::is_same_v<T, MODULEENTRY32>)
	{
		return Module32Next(handle, &entry);
	}
}
