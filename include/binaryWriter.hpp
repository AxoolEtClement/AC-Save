//
// Created by clement on 18/11/2026.
//

#ifndef AC_SAVE_BINARYWRITER_HPP
#define AC_SAVE_BINARYWRITER_HPP
#pragma once

#include <algorithm>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

typedef std::vector<std::byte> Bytes;

class BinaryWriter
{
public:
    void WriteU32(std::uint32_t value);
    void WriteU8(std::uint8_t value);
    void WriteString(std::string_view text);

    void Save(const std::string& path) const
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        if (!file)
            throw std::runtime_error("Cannot open file");

        for (std::byte byte : buffer)
        {
            file.put(static_cast<char>(std::to_integer<unsigned char>(byte)));
        }
    }

private:
    Bytes buffer;
};

#include "binaryWriter.inl"

#endif //AC_SAVE_BINARYWRITER_HPP
