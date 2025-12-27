#include <iostream>
#include <memory>
#include <ProcessManager.h>
#include <string>

int main(int, char* [])
{
	try
	{
		std::unique_ptr<ProcessManager> pm = std::make_unique<ProcessManager>(L"ConsoleAppTarget.exe");

		auto& modules = pm->GetProcessModules();

		for (auto& m : modules)
		{
			std::wcout << m.szExePath << std::endl;
		}
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl << GetLastError();
	}

	return 0;
}
