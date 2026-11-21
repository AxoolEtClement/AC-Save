#pragma once

#include "binary_writer/assetId.hpp"

#include <filesystem>
#include <ostream>
#include <span>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace AC_Save
{
    [[nodiscard]] std::uint32_t crc32(std::span<const std::byte> bytes) noexcept;

    class ArchiveWriter
    {
    public:
        template <writable T>
        void addAsset(AssetId id, AssetType type, std::uint16_t schemaVersion, const T& value)
        {
            if (!id.valid())
                throw std::invalid_argument("Asset id 0 is reserved");
            if (!isKnownType(type))
                throw std::invalid_argument("Unknown asset type");
            BinaryWriter payload;
            write(payload, value);
            const auto bytes = payload.view();
            if (ids.contains(id))
                throw std::invalid_argument("Duplicate asset id");
            assets.push_back({id, type, schemaVersion, Bytes(bytes.begin(), bytes.end())});
            ids.insert(id);
        }

        [[nodiscard]] std::vector<AssetEntry> entries() const;
        void writeTo(std::ostream& output) const;
        void save(const std::filesystem::path& path) const;

    private:
        struct PendingAsset
        {
            AssetId id;
            AssetType type;
            std::uint16_t schemaVersion;
            Bytes data;
        };

        std::vector<PendingAsset> assets;
        std::unordered_set<AssetId> ids;
    };
}

#include "binary_writer/archiveWriter.inl"
