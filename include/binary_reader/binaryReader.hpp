#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <bit>
#include <concepts>
#include <stdexcept>

typedef std::vector<std::byte> Bytes;

namespace AC_Save
{
    class BinaryReader
    {
    public:
        BinaryReader() = default;
        explicit BinaryReader(std::span<const std::byte> bytes) noexcept;
        ~BinaryReader() = default;

        void setView(std::span<const std::byte> bytes) noexcept;
        [[nodiscard]] std::size_t remaining() const noexcept;

        [[nodiscard]] Bytes readBytesCount(std::size_t count);

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

        std::string readString();
    private:
        template<std::unsigned_integral T>
        T readUI();
        std::span<const std::byte> m_bytes;
        std::size_t m_cursor{};
    };
}

#include "binaryReader.inl"