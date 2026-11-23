#include "writer/archiveWriter.hpp"
#include "reader/archiveReader.hpp"

#include <chrono>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <string>
#include <vector>

namespace
{
    struct MaterialV1
    {
        std::string name;
        std::vector<float> coefficients;
    };

    struct Material
    {
        std::string name;
        std::vector<float> coefficients;
        float roughness = 0.5;
    };

    struct LibraryEntry
    {
        std::string title;
        std::uint32_t year{};
        bool available{};
    };
}

template <>
struct AC_Save::Serializer<Material>
{
    static void write(AC_Save::BinaryWriter& writer, const Material& value)
    {
        AC_Save::write(writer, value.name);
        AC_Save::write(writer, value.coefficients);
        AC_Save::write(writer, value.roughness);
    }
};

template <>
struct AC_Save::Serializer<MaterialV1>
{
    static void write(AC_Save::BinaryWriter& writer, const MaterialV1& value)
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

template <>
struct AC_Save::Deserializer<Material>
{
    [[nodiscard]] static Material read(AC_Save::BinaryReader& reader, std::uint16_t schemaVersion)
    {
        Material value{};
        if (schemaVersion == 1)
        {
            value.name = AC_Save::read<std::string>(reader);
            value.coefficients = AC_Save::read<std::vector<float>>(reader);
        }
        else if (schemaVersion == 2)
        {
            value.name = AC_Save::read<std::string>(reader);
            value.coefficients = AC_Save::read<std::vector<float>>(reader);
            value.roughness = AC_Save::read<float>(reader);
        }
        else
        {
            throw std::runtime_error("Unsupported schemaVersion: " + std::to_string(schemaVersion));
        }
        return value;
    }
};

template <>
struct AC_Save::Deserializer<LibraryEntry>
{
    [[nodiscard]] static LibraryEntry read(AC_Save::BinaryReader& reader)
    {
        LibraryEntry value;
        value.title = AC_Save::read<std::string>(reader);
        value.year = AC_Save::read<std::uint32_t>(reader);
        value.available = AC_Save::read<bool>(reader);
        return value;
    }
};

namespace
{
    void printMaterial(const Material& m)
    {
        std::cout << "  name: " << m.name << "\n  coefficients: [";
        for (std::size_t i = 0; i < m.coefficients.size(); ++i)
        {
            if (i > 0)
                std::cout << ", ";
            std::cout << m.coefficients[i];
        }
        std::cout << "]\n";
        std::cout << "roughness: " << m.roughness << "\n";
    }

    void printLibraryEntry(const LibraryEntry& entry)
    {
        std::cout << "  title: " << entry.title
                  << "\n  year: " << entry.year
                  << "\n  available: " << (entry.available ? "true" : "false") << '\n';
    }

    void runMeasurement(std::size_t assetCount, const LibraryEntry& library)
    {
        const std::filesystem::path archivePath =
            "benchmark-" + std::to_string(assetCount) + ".acsa";

        AC_Save::ArchiveWriter writer;
        for (std::size_t i = 0; i < assetCount; ++i)
        {
            writer.addAsset(
                AC_Save::makeAssetId("library " + std::to_string(i)),
                AC_Save::AssetType::Mesh,
                1,
                library);
        }

        const auto saveStart = std::chrono::steady_clock::now();
        writer.save(archivePath);
        const auto saveEnd = std::chrono::steady_clock::now();

        const auto saveTime =
            std::chrono::duration<double, std::milli>(saveEnd - saveStart).count();
        const std::uint64_t archiveSize =
            std::filesystem::file_size(archivePath);

        const auto loadStart = std::chrono::steady_clock::now();
        AC_Save::ArchiveReader reader(archivePath);
        const auto loadEnd = std::chrono::steady_clock::now();

        const auto loadTime =
            std::chrono::duration<double, std::milli>(loadEnd - loadStart).count();

        std::cout << "Benchmark " << assetCount << " assets\n"
                  << "  archive size: " << archiveSize << " bytes\n"
                  << "  save time: " << saveTime << " ms\n"
                  << "  load time: " << loadTime << " ms\n";
    }
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    const MaterialV1 material1{
        .name = std::string{"metal \xE2\x98\x85"},
        .coefficients = {0.8f, 0.6f, 0.2f}
    };
    const Material material2{
        .name = std::string{"metal \xE2\x98\x85"},
        .coefficients = {0.8f, 0.6f, 0.2f},
        .roughness = 0.8f
    };
    const LibraryEntry library{
        .title = "Binary archives",
        .year = 2026,
        .available = true
    };
    constexpr auto materialId = AC_Save::makeAssetId("material");
    constexpr auto libraryId = AC_Save::makeAssetId("library");

    // 1. writing to archives
    {
        AC_Save::ArchiveWriter archive;
        archive.addAsset(materialId,
                         AC_Save::AssetType::Texture, 2, material2);
        archive.addAsset(libraryId,
                         AC_Save::AssetType::Mesh, 1, library);
        archive.save("general-purpose.acsa");
    }

    {
        AC_Save::ArchiveWriter archive;
        archive.addAsset(materialId,
                          AC_Save::AssetType::Texture, 1, material1);
        archive.save("material-v1.acsa");
    }

    {
        AC_Save::ArchiveWriter archive;
        archive.addAsset(materialId,
                          AC_Save::AssetType::Texture, 2, material2);
        archive.save("material-v2.acsa");
    }

    std::cout << "Wrote general-purpose.acsa and material-v1/v2 archives\n";

    // 2. Read general archive
    {
        AC_Save::ArchiveReader reader("general-purpose.acsa");
        std::cout << "general-purpose.acsa -> version: " << reader.version()
                  << ", entries: " << reader.entries().size() << '\n';

        if (const auto* matEntry = reader.findEntry(materialId))
        {
            std::cout << "[material] id=" << matEntry->id.raw()
                      << ", schemaVersion=" << matEntry->schemaVersion
                      << ", size=" << matEntry->size << '\n';
            printMaterial(reader.readAsset<Material>(materialId));
        }

        if (const auto* libEntry = reader.findEntry(libraryId))
        {
            std::cout << "[library] id=" << libEntry->id.raw()
                      << ", schemaVersion=" << libEntry->schemaVersion
                      << ", size=" << libEntry->size << '\n';
            printLibraryEntry(reader.readAsset<LibraryEntry>(libraryId));
        }

        std::cout << '\n';
    }

    // 3. Read and print schema versions from material v1 and v2
    {
        AC_Save::ArchiveReader v1Reader("material-v1.acsa");
        if (const auto* v1Entry = v1Reader.findEntry(materialId))
        {
            std::cout << "material-v1.acsa -> schemaVersion=" << v1Entry->schemaVersion << '\n';
            printMaterial(v1Reader.readAsset<Material>(materialId));
        }
    }

    {
        AC_Save::ArchiveReader v2Reader("material-v2.acsa");
        if (const auto* v2Entry = v2Reader.findEntry(materialId))
        {
            std::cout << "material-v2.acsa -> schemaVersion=" << v2Entry->schemaVersion << '\n';
            printMaterial(v2Reader.readAsset<Material>(materialId));
        }
    }

    runMeasurement(100, library);
    runMeasurement(1000, library);

    return 0;
}
