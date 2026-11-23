#pragma once

namespace AC_Save
{
    inline ArchiveReader::ArchiveReader(const std::filesystem::path& path)
    {
        open(path);
    }

    inline void ArchiveReader::open(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw std::runtime_error("Cannot open file");

        const auto fileSize = file.tellg();
        if (fileSize < 0)
            throw std::runtime_error("Failed to determine file size");

        Bytes fileBytes(static_cast<std::size_t>(fileSize));
        file.seekg(0, std::ios::beg);
        if (!file.read(reinterpret_cast<char*>(fileBytes.data()),
                       static_cast<std::streamsize>(fileBytes.size())))
        {
            throw std::runtime_error("Failed to read file");
        }

        loadFromBytes(std::move(fileBytes));
    }

    inline void ArchiveReader::loadFromBytes(Bytes data)
    {
        m_buffer = std::move(data);
        m_version = 0;
        m_entries.clear();
        m_indexById.clear();
        parseAndValidate();
    }

    inline bool ArchiveReader::contains(AssetId id) const noexcept
    {
        return m_indexById.contains(id);
    }

    inline const AssetEntry* ArchiveReader::findEntry(AssetId id) const noexcept
    {
        const auto it = m_indexById.find(id);
        if (it == m_indexById.end())
            return nullptr;
        return &m_entries[it->second];
    }

    inline const AssetEntry& ArchiveReader::requireEntry(AssetId id) const
    {
        const AssetEntry* entry = findEntry(id);
        if (!entry)
            throw std::out_of_range("Asset ID not found in archive");
        return *entry;
    }

    inline std::span<const std::byte> ArchiveReader::payloadView(AssetId id) const
    {
        const AssetEntry& entry = requireEntry(id);

        if (entry.offset > m_buffer.size() || entry.size > m_buffer.size() - entry.offset)
            throw std::runtime_error("Asset payload range exceeds archive size");

        const auto payloadStart = static_cast<std::size_t>(entry.offset);
        const auto payloadSize = static_cast<std::size_t>(entry.size);
        const std::span<const std::byte> span(m_buffer.data() + payloadStart, payloadSize);

        const std::uint32_t actualCrc = crc32(span);
        if (actualCrc != entry.crc)
            throw std::runtime_error("Asset CRC-32 mismatch");

        return span;
    }

    inline Bytes ArchiveReader::readAssetBytes(AssetId id) const
    {
        const auto span = payloadView(id);
        return Bytes(span.begin(), span.end());
    }

    template<readable T>
    T ArchiveReader::readAsset(AssetId id) const
    {
        BinaryReader payloadReader(payloadView(id));
        return read<T>(payloadReader);
    }

    inline void ArchiveReader::parseAndValidate()
    {
        if (m_buffer.size() < kArchiveHeaderSize)
            throw std::runtime_error("Archive is smaller than header size");

        // Header layout (32 bytes total):
        // 4 magic + 2 version + 2 reserved + 4 assetCount + 8 directoryOffset + 8 directorySize + 4 crc
        constexpr std::size_t kChecksumOffset = 28;
        if (m_buffer.size() < kChecksumOffset + sizeof(std::uint32_t))
            throw std::runtime_error("Archive is too small for header checksum");

        const std::span<const std::byte> archiveSpan{m_buffer};
        BinaryReader headerReader(archiveSpan);

        const Bytes magicBytes = headerReader.readBytesCount(kArchiveMagic.size());
        for (std::size_t i = 0; i < kArchiveMagic.size(); ++i)
        {
            if (magicBytes[i] != kArchiveMagic[i])
                throw std::runtime_error("Invalid archive magic bytes");
        }

        m_version = headerReader.readU16();
        if (m_version != kArchiveVersion)
            throw std::runtime_error("Unsupported archive version: " + std::to_string(m_version));

        const std::uint16_t reserved = headerReader.readU16();
        (void)reserved; // Reserved field currently written as 0 by ArchiveWriter

        const std::uint32_t assetCount = headerReader.readU32();
        const std::uint64_t directoryOffset = headerReader.readU64();
        const std::uint64_t directorySize = headerReader.readU64();
        const std::uint32_t storedArchiveCrc = headerReader.readU32();

        // set checksum's bytes to 0 to match the checksum calculated before writing
        std::array<std::byte, sizeof(std::uint32_t)> originalCrcBytes{};
        for (std::size_t i = 0; i < originalCrcBytes.size(); ++i)
        {
            originalCrcBytes[i] = m_buffer[kChecksumOffset + i];
            m_buffer[kChecksumOffset + i] = std::byte{0};
        }

        const std::uint32_t computedArchiveCrc = crc32(m_buffer);

        // set file's checksum bytes back into the buffer
        for (std::size_t i = 0; i < originalCrcBytes.size(); ++i)
        {
            m_buffer[kChecksumOffset + i] = originalCrcBytes[i];
        }

        if (computedArchiveCrc != storedArchiveCrc)
            throw std::runtime_error("Archive checksum mismatch");

        if (directoryOffset > m_buffer.size() || directorySize > m_buffer.size() - directoryOffset)
            throw std::runtime_error("Directory range exceeds archive size");

        if (static_cast<std::uint64_t>(assetCount) > std::numeric_limits<std::uint64_t>::max() / kAssetEntrySize)
            throw std::runtime_error("Asset count overflow in directory size calculation");

        const std::uint64_t expectedDirectorySize = static_cast<std::uint64_t>(assetCount) * kAssetEntrySize;
        if (directorySize != expectedDirectorySize)
            throw std::runtime_error("Directory size does not match asset count * entry size");

        if (directoryOffset < kArchiveHeaderSize)
            throw std::runtime_error("Directory offset overlaps archive header");

        const std::span<const std::byte> dirSpan(
            m_buffer.data() + static_cast<std::size_t>(directoryOffset),
            static_cast<std::size_t>(directorySize));
        BinaryReader dirReader(dirSpan);

        m_entries.reserve(assetCount);
        m_indexById.reserve(assetCount);

        for (std::uint32_t i = 0; i < assetCount; ++i)
        {
            AssetEntry entry{};
            entry.id = AssetId{dirReader.readU64()};
            entry.type = static_cast<AssetType>(dirReader.readU32());
            entry.schemaVersion = dirReader.readU16();
            entry.offset = dirReader.readU64();
            entry.size = dirReader.readU64();
            entry.crc = dirReader.readU32();

            if (!entry.id.valid())
                throw std::runtime_error("Archive contains invalid asset ID 0");

            if (!isKnownType(entry.type))
                throw std::runtime_error("Archive contains unknown asset type");

            if (m_indexById.contains(entry.id))
                throw std::runtime_error("Duplicate asset ID in archive directory");

            if (entry.offset < kArchiveHeaderSize ||
                entry.offset > directoryOffset ||
                entry.size > directoryOffset - entry.offset)
            {
                throw std::runtime_error("Asset payload outside valid payload region");
            }

            m_indexById.emplace(entry.id, m_entries.size());
            m_entries.push_back(entry);
        }
    }
}
