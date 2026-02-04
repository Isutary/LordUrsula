#include "pch.h"

#include <ProcessManager.h>
#include <PortableExecutable.h>

int main(int, char* [])
{
	try
	{
		std::unique_ptr<ProcessManager> processManager = std::make_unique<ProcessManager>(L"ConsoleAppTarget.exe");

		processManager->LoadTargetLibrary(L"C:\\Projects\\Visual Studio\\TargetDLL\\x64\\Debug\\TargetDLL.dll");
	
		std::unique_ptr<PortableExecutable> portableExecutable = std::make_unique<PortableExecutable>(processManager->ReadBaseModuleMemory());
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl << "Last error code: " << GetLastError();
	}

	return 0;
}
