#pragma once

#include <string>
#include <format>
#include <Exception.h>

namespace Exceptions
{
	class WindowsException : public Exception
	{
	public:
		explicit WindowsException(std::wstring message)
			: Exception(message), _errorCode(GetLastError())
		{
		};
		template<typename... Args>
		explicit WindowsException(std::wformat_string<Args...> fmt, Args&&... args)
			: WindowsException(std::format(fmt, std::forward<Args>(args)...)) {};
		const unsigned long GetErrorCode() const noexcept { return _errorCode; };
	private:
		unsigned long _errorCode;
	};
}
