#pragma once

#include <cstdint>
#include <cstddef>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>
#include <span>

namespace AC_Save
{
    typedef std::vector<std::byte> Bytes;

    template <class T> struct Serializer;

    class BinaryWriter
    {
    public:

        // unsigned int :
        void writeU8(std::uint8_t value);
        void writeU16(std::uint16_t value);
        void writeU32(std::uint32_t value);
        void writeU64(std::uint64_t value);

        // signed int :
        void writeI8(std::int8_t value);
        void writeI16(std::int16_t value);
        void writeI32(std::int32_t value);
        void writeI64(std::int64_t value);

        // other useful types:
        void writeBool(bool value);
        void writeF32(float value);
        void writeF64(double value);

        void writeBytes(std::span<const std::byte> data);
        void writeString(std::string_view text);

        // patching function for the header :
        void patchU32(std::size_t offset, std::uint32_t value);
        void patchU64(std::size_t offset, std::uint64_t value);

        void writeTo(std::ostream& out) const;
        void save(const std::filesystem::path& path) const;

    public:

        [[nodiscard]]std::size_t size() const { return buffer.size(); }
        [[nodiscard]]std::span<const std::byte> view() const { return buffer; }
        void reserve(std::size_t const n) { buffer.reserve(n); }

    private:

        template <std::unsigned_integral T> void writeLE(T value);
        template <std::unsigned_integral T> void patchLE(std::size_t offset, T value);

    private:
        Bytes buffer;
    };
}
#include "writer/binaryWriter.inl"
