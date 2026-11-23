#pragma once

namespace AC_Save
{
    inline void BinaryReader::load(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary|std::ios::ate);
        if (!file)
            throw std::runtime_error("Cannot open file");

        const auto fileSize = file.tellg();
        if (fileSize < 0)
            throw std::runtime_error("failed to determine file size");

        m_bytes.resize(fileSize);
        m_cursor = 0;

        file.seekg(0, std::ios::beg);
        if (!file.read(reinterpret_cast<char*>(m_bytes.data()), static_cast<std::streamsize>(m_bytes.size())))
            throw std::runtime_error("failed to read file");
    }

    inline std::size_t BinaryReader::remaining() const noexcept
    {
        return m_cursor <= m_bytes.size() ? m_bytes.size() - m_cursor : 0;
    }

    inline std::uint8_t BinaryReader::readU8() { return readUI<uint8_t>(); }
    inline std::uint16_t BinaryReader::readU16() { return readUI<uint16_t>(); }
    inline std::uint32_t BinaryReader::readU32() { return readUI<uint32_t>(); }
    inline std::uint64_t BinaryReader::readU64() { return readUI<uint64_t>(); }
    inline std::int8_t BinaryReader::readI8() { return std::bit_cast<int8_t>(readUI<uint8_t>()); }

    inline std::int16_t BinaryReader::readI16() { return std::bit_cast<int16_t>(readUI<uint16_t>()); }

    inline std::int32_t BinaryReader::readI32() { return std::bit_cast<int32_t>(readUI<uint32_t>()); }

    inline std::int64_t BinaryReader::readI64() { return std::bit_cast<int64_t>(readUI<uint64_t>()); }

    inline bool BinaryReader::readBool()
    {
        const auto raw = readUI<std::uint8_t>();
        if (raw > 1)
            throw std::runtime_error("Invalid boolean value in binary stream");

        return raw != 0;
    }

    inline float BinaryReader::readF32() { return std::bit_cast<float>(readUI<uint32_t>()); }
    inline double BinaryReader::readF64() { return std::bit_cast<double>(readUI<uint64_t>()); }

    inline Bytes BinaryReader::readBytes()
    {
        std::uint32_t byteSize{ readUI<std::uint32_t>() };
        if (m_cursor > m_bytes.size() || m_bytes.size() - m_cursor < byteSize)
            throw std::runtime_error("File is too small to read data of size " + std::to_string(byteSize));

        Bytes result{};
        result.resize(byteSize);
        for (std::uint32_t i = 0; i < byteSize; ++i)
        {
            result[i] = m_bytes[m_cursor++];
        }
        return result;
    }

    inline std::string BinaryReader::readString()
    {
        std::uint32_t byteSize{ readUI<std::uint32_t>() };
        if (m_cursor > m_bytes.size() || m_bytes.size() - m_cursor < byteSize)
            throw std::runtime_error("File is too small to read string");

        std::string result{};
        result.resize(byteSize);
        for (std::uint32_t i = 0; i < byteSize; ++i)
        {
            result[i] = std::to_integer<char>(m_bytes[m_cursor++]);
        }
        return result;
    }

    template<std::unsigned_integral T>
    T BinaryReader::readUI()
    {
        if (m_cursor > m_bytes.size() || m_bytes.size() - m_cursor < sizeof(T))
            throw std::runtime_error("File is too small to read data of size " + std::to_string(sizeof(T)));

        T result{};
        for (size_t i = 0; i < sizeof(T); ++i)
        {
            const auto byteValue = std::to_integer<T>(m_bytes[m_cursor++]);
            result |= byteValue << i * 8;
        }

        return result;
    }
}
