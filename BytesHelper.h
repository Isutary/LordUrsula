#pragma once

#include <vector>

namespace Helpers
{
    template<typename T>
    std::vector<std::byte> ToBytes(T value)
    {
        std::vector<std::byte> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        return bytes;
    }
}
