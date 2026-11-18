#pragma once
#include "binaryWriter.hpp"

inline void BinaryWriter::WriteU32(std::uint32_t value)
{
    for (int i = 0; i < 4; ++i)
    {
        buffer.push_back(static_cast<std::byte>(value & 0xFF));

        value >>= 8;
    }
}

inline void BinaryWriter::WriteU8(std::uint8_t value)
{
    buffer.push_back(std::byte{value});
}

inline void BinaryWriter::WriteString(std::string_view text)
{
    if (text.size() > std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("String too large");

    WriteU32(static_cast<std::uint32_t>(text.size()));

    for (const char& value : text)
    {
        WriteU8(static_cast<std::uint8_t>(value));
    }
}