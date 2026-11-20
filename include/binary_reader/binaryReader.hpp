#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

typedef std::vector<std::byte> Bytes;

namespace AC_Save
{
    class BinaryReader
    {
    public:
        BinaryReader() = default;
        ~BinaryReader() = default;

        void load(const std::filesystem::path& path);

        std::uint8_t readU8();
        std::uint16_t readU16();
        std::uint32_t readU32();
        std::uint64_t readU64();

        std::int8_t readI8();
        std::int16_t readI16();
        std::int32_t readI32();
        std::int64_t readI64();

        bool readBool();
        float readF32();
        double readF64();

        Bytes readBytes();
        std::string readString();
    private:
        template<std::unsigned_integral T>
        T readUI();
        Bytes m_bytes{};
        std::size_t m_cursor{};
    };
}

#include "binaryReader.inl"