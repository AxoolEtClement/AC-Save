#pragma once

namespace AC_Save
{
    inline void BinaryReader::load(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary|std::ios::ate);
        if (!file)
            throw std::runtime_error("Cannot open file");

        m_bytes.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(m_bytes.data(), m_bytes.size());
    }

    inline std::uint8_t BinaryReader::readU8()
    {
    }

    inline std::uint16_t BinaryReader::readU16()
    {
    }

    inline std::uint32_t BinaryReader::readU32()
    {
    }

    inline std::uint64_t BinaryReader::readU64()
    {
    }

    std::int8_t BinaryReader::readI8()
    {
    }

    inline std::int16_t BinaryReader::readI16()
    {
    }

    inline std::int32_t BinaryReader::readI32()
    {
    }

    inline std::int64_t BinaryReader::readI64()
    {
    }

    inline bool BinaryReader::readBool()
    {
    }

    inline float BinaryReader::readF32()
    {
    }

    inline double BinaryReader::readF64()
    {
    }

    inline std::string & BinaryReader::readString()
    {
    }

    template<std::unsigned_integral T>
    T BinaryReader::readLE()
    {
        for (size_t i = m_cursor; i < m_cursor + sizeof(T); ++i)
        {

        }
    }
}
ziziiiiiiiiiiiiiiii