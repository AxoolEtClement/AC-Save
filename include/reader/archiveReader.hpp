#pragma once

#include "writer/assetId.hpp"
#include "binaryReader.hpp"
#include "Deserializer.hpp"
#include "writer/archiveWriter.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <unordered_map>
#include <vector>
#include <limits>
#include <stdexcept>
#include <string>

namespace AC_Save
{
    class ArchiveReader
    {
    public:
        ArchiveReader() = default;
        explicit ArchiveReader(const std::filesystem::path& path);

        void open(const std::filesystem::path& path);
        void loadFromBytes(Bytes data);

        [[nodiscard]] bool contains(AssetId id) const noexcept;
        [[nodiscard]] const AssetEntry* findEntry(AssetId id) const noexcept;
        [[nodiscard]] const std::vector<AssetEntry>& entries() const noexcept { return m_entries; }
        [[nodiscard]] std::uint16_t version() const noexcept { return m_version; }

        // Returns raw payload bytes for an asset after verifying its per-asset CRC-32.
        [[nodiscard]] Bytes readAssetBytes(AssetId id) const;

        // Verifies per-asset CRC-32 and deserializes the payload into `T`.
        template <readable T>
        [[nodiscard]] T readAsset(AssetId id) const;

    private:
        void parseAndValidate();
        [[nodiscard]] const AssetEntry& requireEntry(AssetId id) const;
        [[nodiscard]] std::span<const std::byte> payloadView(AssetId id) const;

        Bytes m_buffer{};
        std::uint16_t m_version = 0;
        std::vector<AssetEntry> m_entries{};
        std::unordered_map<AssetId, std::size_t> m_indexById{};
    };
}

#include "archiveReader.inl"