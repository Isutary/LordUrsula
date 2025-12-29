#include <iostream>
#include <memory>
#include <ProcessManager.h>
#include <string>
#include <algorithm>

int main(int, char* [])
{
	try
	{
		std::unique_ptr<ProcessManager> pm = std::make_unique<ProcessManager>(L"ConsoleAppTarget.exe");

		auto& modules = pm->GetProcessModules();

		for (auto& m : modules)
		{
			std::wcout << m.modBaseSize << std::endl;
		}
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl << "Last error code: " << GetLastError();
	}

	return 0;
}
