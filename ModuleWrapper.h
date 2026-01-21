#pragma once

#include <windows.h>
#include <vector>

struct ModuleWrapper
{
	std::vector<BYTE> buffer;
	BYTE* moduleBaseAddress;
};