#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <format>

template<typename T>
concept SnapshotEntry = std::is_same_v<T, PROCESSENTRY32> || std::is_same_v<T, MODULEENTRY32>;

template <SnapshotEntry T>
std::vector<T> ReadSnapshotEntries(DWORD flags, DWORD processId = 0)
{
	HANDLE handle = CreateToolhelp32Snapshot(flags, processId);
	if (handle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error(std::format("Failed to create snapshot for process {0} with flags {1:X}.", processId, flags));
	}

	T entry;
	entry.dwSize = sizeof(entry);

	if (!First(handle, entry))
	{
		CloseHandle(handle);
		throw std::runtime_error(std::format("Failed to find first entry in snapshot for process {0} with flags {1:X}.", processId, flags));
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
	else if constexpr (std::is_same_v<T, MODULEENTRY32>)
	{
		return Module32First(handle, &entry);
	}
	else
	{
		static_assert(false, "Unsupported snapshot type.");
	}
}

template <SnapshotEntry T>
bool Next(HANDLE handle, T& entry)
{
	if constexpr (std::is_same_v<T, PROCESSENTRY32>)
	{
		return Process32Next(handle, &entry);
	}
	else if constexpr (std::is_same_v<T, MODULEENTRY32>)
	{
		return Module32Next(handle, &entry);
	}
	else
	{
		static_assert(false, "Unsupported snapshot type.");
	}
}
