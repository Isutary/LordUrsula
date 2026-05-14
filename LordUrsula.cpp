#include <pch.h>

#include <IProcessManager.h>
#include <ProcessManager.h>
#include <IModuleManager.h>
#include <ModuleManager.h>
#include <PortableExecutable.h>
#include <CoutHelper.h>
#include <CodeBuilder.h>
#include <fstream>
#include <Trampoline.h>

int main(int, char* [])
{
	using IProcessManager = Managers::Interfaces::IProcessManager;
	using ProcessManager = Managers::ProcessManager;
	using IModuleManager = Managers::Interfaces::IModuleManager;
	using ModuleManager = Managers::ModuleManager;
	using PortableExecutable = Models::PortableExecutable;
	using CodeBuilder = Builders::CodeBuilder;
	using Trampoline = Models::Trampoline;

	try
	{
		std::unique_ptr<IProcessManager> processManager = std::make_unique<ProcessManager>(L"ConsoleAppTarget.exe");

		ModuleManager kernelModuleManager {L"kernel32.dll"};
		processManager->LoadRemoteLibrary(L"C:\\Projects\\Visual Studio\\TargetDLL\\x64\\Release\\TargetDLL.dll", kernelModuleManager);

		std::unique_ptr<PortableExecutable> exePortableExecutable = std::make_unique<PortableExecutable>(processManager->ReadModuleMemory(L"ConsoleAppTarget.exe"));
		std::unique_ptr<PortableExecutable> targetDLLPortableExecutable = std::make_unique<PortableExecutable>(processManager->ReadModuleMemory(L"TargetDLL.dll"));

		std::uint32_t hackOffset = targetDLLPortableExecutable->GetExportedFunction("Hack");

		CodeBuilder codeBuilder = Builders::CodeBuilder(*exePortableExecutable);
		Trampoline trampoline = codeBuilder.CreateCallSiteDetour(targetDLLPortableExecutable->GetBaseAddress() + hackOffset);

		processManager->WriteMemory(trampoline.GetTargetFunctionAddress(), trampoline.GetTargetFunction());
		processManager->WriteMemory(trampoline.GetTrampolineAddress(), trampoline.GetTrampolineInstruction());
		processManager->WriteMemory(trampoline.GetCallAddress(), trampoline.GetCallInstruction());
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl << "Last error code: " << GetLastError();
	}

	return 0;
}
