#pragma once

#include "pch.h"

struct ModuleWrapper
{
	std::vector<BYTE> buffer;
	BYTE* moduleBaseAddress;
};