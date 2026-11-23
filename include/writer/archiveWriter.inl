#pragma once

#include <array>
#include <fstream>
#include <limits>
#include <sstream>

namespace AC_Save
{
    inline constexpr std::array<std::byte, 4> kArchiveMagic{
        std::byte{'A'}, std::byte{'C'}, std::byte{'S'}, std::byte{'A'}};
    inline constexpr std::uint16_t kArchiveVersion = 1;
    inline constexpr std::uint64_t kArchiveHeaderSize = 32;

    inline std::uint32_t crc32(std::span<const std::byte> bytes) noexcept
    {
        std::uint32_t crc = 0xFFFFFFFFu;
        for (const std::byte byte : bytes)
        {
            crc ^= std::to_integer<std::uint8_t>(byte);
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ (0xEDB88320u & static_cast<std::uint32_t>(-(crc & 1u)));
        }
        return ~crc;
    }

    inline std::vector<AssetEntry> ArchiveWriter::entries() const
    {
        std::vector<AssetEntry> result;
        result.reserve(assets.size());
        std::uint64_t offset = kArchiveHeaderSize;
        for (const PendingAsset& asset : assets)
        {
            if (offset > std::numeric_limits<std::uint64_t>::max() - asset.data.size())
                throw std::length_error("Archive is too large");
            result.push_back({asset.id, asset.type, asset.schemaVersion, offset,
                              static_cast<std::uint64_t>(asset.data.size()), crc32(asset.data)});
            offset += asset.data.size();
        }
        return result;
    }

    inline void ArchiveWriter::writeTo(std::ostream& output) const
    {
        const auto assetEntries = entries();
        std::uint64_t payloadSize = 0;
        for (const PendingAsset& asset : assets)
        {
            if (payloadSize > std::numeric_limits<std::uint64_t>::max() - asset.data.size())
                throw std::length_error("Archive is too large");
            payloadSize += asset.data.size();
        }
        if (payloadSize > std::numeric_limits<std::uint64_t>::max() - kArchiveHeaderSize)
            throw std::length_error("Archive is too large");
        const std::uint64_t directoryOffset = kArchiveHeaderSize + payloadSize;
        if (assetEntries.size() > std::numeric_limits<std::uint64_t>::max() / kAssetEntrySize)
            throw std::length_error("Directory is too large");
        const std::uint64_t directorySize = assetEntries.size() * kAssetEntrySize;
        if (assetEntries.size() > std::numeric_limits<std::uint32_t>::max())
            throw std::length_error("Too many assets");

        BinaryWriter archive;
        archive.writeBytes(kArchiveMagic);
        archive.writeU16(kArchiveVersion);
        archive.writeU16(0);
        archive.writeU32(static_cast<std::uint32_t>(assetEntries.size()));
        archive.writeU64(directoryOffset);
        archive.writeU64(directorySize);
        const std::size_t checksumOffset = archive.size();
        archive.writeU32(0);
        for (const PendingAsset& asset : assets)
            archive.writeBytes(asset.data);
        for (const AssetEntry& entry : assetEntries)
            write(archive, entry);
        archive.patchU32(checksumOffset, crc32(archive.view()));
        archive.writeTo(output);
    }

    inline void ArchiveWriter::save(const std::filesystem::path& path) const
    {
        BinaryWriter bytes;
        std::ostringstream stream(std::ios::out | std::ios::binary);
        writeTo(stream);
        const std::string data = stream.str();
        bytes.writeBytes(std::as_bytes(std::span<const char>(data.data(), data.size())));
        bytes.save(path);
    }
}
