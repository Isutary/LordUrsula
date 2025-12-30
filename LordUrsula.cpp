#include <iostream>
#include <memory>
#include <string>
#include <algorithm>
#include <span>

#include <ProcessManager.h>
#include <PortableExecutable.h>

int main(int, char* [])
{
	try
	{
		std::unique_ptr<ProcessManager> processManager = std::make_unique<ProcessManager>(L"ConsoleAppTarget.exe");
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl << "Last error code: " << GetLastError();
	}

	return 0;
}
