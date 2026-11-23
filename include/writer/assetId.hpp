#pragma once

#include "writer/coreSerializer.hpp"

#include <compare>
#include <cstdint>
#include <functional>
#include <string_view>

namespace AC_Save
{
    template <class Tag>
    class StrongId
    {
    public:
        using value_type = std::uint64_t;

        constexpr StrongId() = default;
        constexpr explicit StrongId(value_type value) : value(value) {}

        [[nodiscard]] constexpr value_type raw() const { return value; }
        [[nodiscard]] constexpr bool valid() const { return value != 0; }
        friend constexpr auto operator<=>(const StrongId&, const StrongId&) = default;

    private:
        value_type value = 0;
    };

    using AssetId = StrongId<struct AssetTag>;
    using BodyId = StrongId<struct BodyTag>;

    template <class Tag>
    struct Serializer<StrongId<Tag>>
    {
        static void write(BinaryWriter& writer, const StrongId<Tag>& id)
        {
            writer.writeU64(id.raw());
        }
    };

    [[nodiscard]] constexpr AssetId makeAssetId(std::string_view name)
    {
        std::uint64_t hash = 14695981039346656037ull;
        for (const char character : name)
        {
            hash ^= static_cast<unsigned char>(character);
            hash *= 1099511628211ull;
        }
        return AssetId{hash == 0 ? 1 : hash};
    }
}

namespace std
{
    template <class Tag>
    struct hash<AC_Save::StrongId<Tag>>
    {
        size_t operator()(AC_Save::StrongId<Tag> id) const noexcept
        {
            return hash<std::uint64_t>{}(id.raw());
        }
    };
}

namespace AC_Save
{
    enum class AssetType : std::uint32_t
    {
        Unknown = 0,
        Mesh = 1,
        Texture = 2,
        Scene = 3,
    };

    [[nodiscard]] constexpr bool isKnownType(AssetType type)
    {
        return type == AssetType::Mesh || type == AssetType::Texture || type == AssetType::Scene;
    }

    struct AssetEntry
    {
        AssetId id;
        AssetType type = AssetType::Unknown;
        std::uint16_t schemaVersion = 0;
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
        std::uint32_t crc = 0;
    };

    inline constexpr std::size_t kAssetEntrySize = 34;

    template <>
    struct Serializer<AssetEntry>
    {
        static void write(BinaryWriter& writer, const AssetEntry& entry)
        {
            Serializer<AssetId>::write(writer, entry.id);
            Serializer<AssetType>::write(writer, entry.type);
            writer.writeU16(entry.schemaVersion);
            writer.writeU64(entry.offset);
            writer.writeU64(entry.size);
            writer.writeU32(entry.crc);
        }
    };
}
