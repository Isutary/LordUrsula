#pragma once

#include <iostream>

namespace Helpers
{
	template<typename T>
	concept IndexableWithSize = requires(const T container, std::size_t index)
	{
		{ container[index] } -> std::convertible_to<std::byte>;
		{ container.size() } -> std::convertible_to<std::size_t>;
	};

	template<IndexableWithSize Container>
	void Print(const Container& buffer)
	{
		auto restore_flags = std::cout.flags();
		auto restore_fill = std::cout.fill();

		std::cout << std::hex << std::setfill('0') << std::uppercase;
		for (int i = 0; i < buffer.size(); i++)
		{
			if (i != 0 && i % 16 == 0) std::cout << std::endl;
			std::cout << std::setw(2) << static_cast<int>(buffer[i]) << " ";
		}

		std::cout.flags(restore_flags);
		std::cout.fill(restore_fill);
	}
}
