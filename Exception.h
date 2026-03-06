#pragma once

#include <Windows.h>
#include <exception>
#include <string>
#include <format>

namespace Exceptions
{
	class Exception : public std::exception
	{
	public:
		explicit Exception(std::wstring message)
			: _wideMessage(std::move(message)), _narrowMessage(wstring_to_string(message)) 
		{
		};
		template<typename... Args>
		explicit Exception(std::wformat_string<Args...> fmt, Args&&... args)
			: Exception(std::format(fmt, std::forward<Args>(args)...))
		{
		};
		const char* what() const noexcept override { return _narrowMessage.c_str(); };
		const std::wstring& message() const noexcept { return _wideMessage; };
	private:
		std::string _narrowMessage;
		std::wstring _wideMessage;
		static std::string wstring_to_string(const std::wstring& wideMessage)
		{
			if (wideMessage.empty())
			{
				return {};
			}

			std::string narrowMessage(wideMessage.size() * 4, '\0');
			int length = WideCharToMultiByte(CP_UTF8, 0, wideMessage.data(), -1, narrowMessage.data(), static_cast<int>(narrowMessage.size()), nullptr, nullptr);

			if (length == 0)
			{
				return "[Failed to convert wide message to narrow message]";
			}

			narrowMessage.resize(length);
			return narrowMessage;
		}
	};
}
