#include "binary_writer/archiveWriter.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace
{
    struct Material
    {
        std::string name;
        std::vector<float> coefficients;
    };

    struct LibraryEntry
    {
        std::string title;
        std::uint32_t year;
        bool available;
    };
}

template <>
struct AC_Save::Serializer<Material>
{
    static void write(AC_Save::BinaryWriter& writer, const Material& value)
    {
        AC_Save::write(writer, value.name);
        AC_Save::write(writer, value.coefficients);
    }
};

template <>
struct AC_Save::Serializer<LibraryEntry>
{
    static void write(AC_Save::BinaryWriter& writer, const LibraryEntry& value)
    {
        AC_Save::write(writer, value.title);
        AC_Save::write(writer, value.year);
        AC_Save::write(writer, value.available);
    }
};

int main()
{
    AC_Save::ArchiveWriter archive;
    const Material material{std::string{"metal \xE2\x98\x85"}, {0.8f, 0.6f, 0.2f}};
    const LibraryEntry library{"Binary archives", 2026, true};

    archive.addAsset(AC_Save::makeAssetId("material"),
                     AC_Save::AssetType::Texture, 1, material);
    archive.addAsset(AC_Save::makeAssetId("library"),
                     AC_Save::AssetType::Mesh, 1, library);
    archive.save("general-purpose.acsa");

    AC_Save::ArchiveWriter schemaV1;
    schemaV1.addAsset(AC_Save::makeAssetId("material"),
                      AC_Save::AssetType::Texture, 1, material);
    schemaV1.save("material-v1.acsa");

    AC_Save::ArchiveWriter schemaV2;
    schemaV2.addAsset(AC_Save::makeAssetId("material"),
                      AC_Save::AssetType::Texture, 2, material);
    schemaV2.save("material-v2.acsa");

    std::cout << "Wrote general-purpose.acsa and material-v1/v2 archives\n";
}
