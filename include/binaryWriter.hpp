//
// Created by clement on 18/11/2026.
//

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
    void Save(const std::string& path) const;

private:
    Bytes buffer;
};

#include "binaryWriter.inl"