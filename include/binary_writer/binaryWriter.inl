#pragma once
#include <bit>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <system_error>

namespace AC_Save
{
    template<std::unsigned_integral T>
    inline void BinaryWriter::writeLE(T value)
    {
        const std::size_t pos = buffer.size();
        buffer.resize(pos + sizeof(T));
        for (std::size_t i = 0 ; i < sizeof(T); ++i)
            buffer[pos + i] = static_cast<std::byte>((value >> (8 * i)) & 0xFF) ;
    }

    template <std::unsigned_integral T>
    void BinaryWriter::patchLE(std::size_t offset, T value)
    {
        // we don't use addition to prevent eventual overflow
        if (offset > buffer.size() || buffer.size() - offset < sizeof(T))
            throw std::out_of_range("Patch out of range");
        for (std::size_t i = 0; i < sizeof(T); ++i)
            buffer[offset + i] = static_cast<std::byte>((value >> (8 * i)) & 0xFF);
    }

    inline void BinaryWriter::writeU8(std::uint8_t v)  { writeLE(v); }
    inline void BinaryWriter::writeU16(std::uint16_t v){ writeLE(v); }
    inline void BinaryWriter::writeU32(std::uint32_t v){ writeLE(v); }
    inline void BinaryWriter::writeU64(std::uint64_t v){ writeLE(v); }

    inline void BinaryWriter::writeI8(std::int8_t v)   { writeU8(static_cast<std::uint8_t>(v)); }
    inline void BinaryWriter::writeI16(std::int16_t v) { writeU16(static_cast<std::uint16_t>(v)); }
    inline void BinaryWriter::writeI32(std::int32_t v) { writeU32(static_cast<std::uint32_t>(v)); }
    inline void BinaryWriter::writeI64(std::int64_t v) { writeU64(static_cast<std::uint64_t>(v)); }

    inline void BinaryWriter::writeBool(bool v) { writeU8(v ? 1 : 0); }
    inline void BinaryWriter::writeF32(float v)  { writeU32(std::bit_cast<std::uint32_t>(v)); }
    inline void BinaryWriter::writeF64(double v) { writeU64(std::bit_cast<std::uint64_t>(v)); }

    inline void BinaryWriter::writeBytes(std::span<const std::byte> data)
    {
        buffer.insert(buffer.end(), data.begin(), data.end());
    }

    inline void BinaryWriter::writeString(std::string_view text)
    {
        if (text.size() > std::numeric_limits<std::uint32_t>::max())
            throw std::length_error("String too large");

        writeU32(static_cast<std::uint32_t>(text.size()));
        writeBytes(std::as_bytes(std::span<const char>{text.data(), text.size()}));
    }

    inline void BinaryWriter::patchU32(std::size_t o, std::uint32_t v) { patchLE(o, v); }
    inline void BinaryWriter::patchU64(std::size_t o, std::uint64_t v) { patchLE(o, v); }

    inline void BinaryWriter::writeTo(std::ostream& out) const
    {
        out.write(reinterpret_cast<const char*>(buffer.data()),
                  static_cast<std::streamsize>(buffer.size()));
        out.flush();
        if (!out)
            throw std::runtime_error("Write failed");
    }

    inline void BinaryWriter::save(const std::filesystem::path& path) const
    {
        namespace fs = std::filesystem;
        fs::path tmp = path;
        tmp += ".tmp";

        std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
        if (!file)
            throw std::runtime_error("Cannot open file");

        try
        {
            writeTo(file);
            file.close();
            if (file.fail())
                throw std::runtime_error("Close failed");
        }
        catch (...)
        {
            file.close();
            std::error_code ec;
            fs::remove(tmp, ec);
            throw;
        }

        std::error_code ec;
        fs::rename(tmp, path, ec);
        if (ec)
        {
            fs::remove(tmp, ec);
            throw std::runtime_error("Cannot finalize file");
        }
    }
}