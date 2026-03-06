#include "pch.h"

#include <IProcessManager.h>
#include <ProcessManager.h>
#include <IModuleManager.h>
#include <ModuleManager.h>
#include <PortableExecutable.h>

int main(int, char* [])
{
	using IProcessManager = Managers::Interfaces::IProcessManager;
	using ProcessManager = Managers::ProcessManager;
	using IModuleManager = Managers::Interfaces::IModuleManager;
	using ModuleManager = Managers::ModuleManager;
	try
	{
		std::unique_ptr<IProcessManager> processManager = std::make_unique<ProcessManager>(L"ConsoleAppTarget.exe");
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl << "Last error code: " << GetLastError();
	}

	return 0;
}
