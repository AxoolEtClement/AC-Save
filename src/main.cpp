#include "binary_writer/archiveWriter.hpp"
#include "binary_reader/archiveReader.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

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

template <>
struct AC_Save::Deserializer<Material>
{
    [[nodiscard]] static Material read(AC_Save::BinaryReader& reader)
    {
        Material value;
        value.name = AC_Save::read<std::string>(reader);
        value.coefficients = AC_Save::read<std::vector<float>>(reader);
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
    }

    void printLibraryEntry(const LibraryEntry& entry)
    {
        std::cout << "  title: " << entry.title
                  << "\n  year: " << entry.year
                  << "\n  available: " << (entry.available ? "true" : "false") << '\n';
    }

    std::uint64_t privateMemoryUsage()
    {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS memory{};
        if (GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)))
            return memory.PagefileUsage;
#endif
        return 0;
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

        const std::uint64_t memoryBeforeSave = privateMemoryUsage();
        const auto saveStart = std::chrono::steady_clock::now();
        writer.save(archivePath);
        const auto saveEnd = std::chrono::steady_clock::now();
        const std::uint64_t memoryAfterSave = privateMemoryUsage();

        const auto saveTime =
            std::chrono::duration<double, std::milli>(saveEnd - saveStart).count();
        const std::uint64_t archiveSize =
            std::filesystem::file_size(archivePath);

        const std::uint64_t memoryBeforeLoad = privateMemoryUsage();
        const auto loadStart = std::chrono::steady_clock::now();
        AC_Save::ArchiveReader reader(archivePath);
        const auto loadEnd = std::chrono::steady_clock::now();
        const std::uint64_t memoryAfterLoad = privateMemoryUsage();

        const auto loadTime =
            std::chrono::duration<double, std::milli>(loadEnd - loadStart).count();

        const auto memoryDelta = [](std::uint64_t before, std::uint64_t after)
        {
            return after >= before ? after - before : 0;
        };

        std::cout << "Benchmark " << assetCount << " assets\n"
                  << "  archive size: " << archiveSize << " bytes\n"
                  << "  save time: " << saveTime << " ms\n"
                  << "  load time: " << loadTime << " ms\n"
                  << "  temporary save memory: "
                  << memoryDelta(memoryBeforeSave, memoryAfterSave) << " bytes\n"
                  << "  temporary load memory: "
                  << memoryDelta(memoryBeforeLoad, memoryAfterLoad) << " bytes\n";
    }
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    AC_Save::ArchiveWriter archive;
    const Material material{std::string{"metal \xE2\x98\x85"}, {0.8f, 0.6f, 0.2f}};
    const LibraryEntry library{"Binary archives", 2026, true};

    constexpr auto materialId = AC_Save::makeAssetId("material");
    constexpr auto libraryId = AC_Save::makeAssetId("library");

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

    // 3. Read and print schema versions from v1 and v2
    AC_Save::ArchiveReader v1Reader("material-v1.acsa");
    AC_Save::ArchiveReader v2Reader("material-v2.acsa");

    if (const auto* v1Entry = v1Reader.findEntry(materialId))
    {
        std::cout << "material-v1.acsa -> schemaVersion=" << v1Entry->schemaVersion << '\n';
        printMaterial(v1Reader.readAsset<Material>(materialId));
    }

    if (const auto* v2Entry = v2Reader.findEntry(materialId))
    {
        std::cout << "material-v2.acsa -> schemaVersion=" << v2Entry->schemaVersion << '\n';
        printMaterial(v2Reader.readAsset<Material>(materialId));
    }

    runMeasurement(100, library);
    runMeasurement(1000, library);

    return 0;
}
