#include "binary_writer/archiveWriter.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace AC_Save;

namespace
{
    int failures = 0;

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

    void expect(bool condition, const char* message)
    {
        if (!condition)
        {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    template <class Function>
    void expectFailure(Function&& function)
    {
        bool failed = false;
        try { function(); } catch (const std::exception&) { failed = true; }
        expect(failed, "operation should fail");
    }

    std::string encode(const ArchiveWriter& archive)
    {
        std::ostringstream output(std::ios::out | std::ios::binary);
        archive.writeTo(output);
        return output.str();
    }

    std::uint16_t readU16(const std::string& data, std::size_t offset)
    {
        if (offset + 2 > data.size())
            throw std::out_of_range("readU16 past archive");
        return static_cast<std::uint16_t>(
            static_cast<unsigned char>(data[offset]) |
            (static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset + 1])) << 8));
    }

    std::uint32_t readU32(const std::string& data, std::size_t offset)
    {
        if (offset + 4 > data.size())
            throw std::out_of_range("readU32 past archive");
        std::uint32_t value = 0;
        for (std::size_t i = 0; i < 4; ++i)
            value |= static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + i])) << (8 * i);
        return value;
    }

    std::uint64_t readU64(const std::string& data, std::size_t offset)
    {
        if (offset + 8 > data.size())
            throw std::out_of_range("readU64 past archive");
        std::uint64_t value = 0;
        for (std::size_t i = 0; i < 8; ++i)
            value |= static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + i])) << (8 * i);
        return value;
    }

    std::uint32_t crcForArchive(const std::string& data)
    {
        std::string copy = data;
        if (copy.size() < kArchiveHeaderSize)
            throw std::out_of_range("archive too short for checksum");
        std::fill(copy.begin() + 28, copy.begin() + 32, '\0');
        const auto bytes = std::as_bytes(std::span<const char>(copy.data(), copy.size()));
        return crc32(bytes);
    }

    class BadOutputStream : public std::ostream
    {
    public:
        BadOutputStream() : std::ostream(nullptr) {}
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
    ArchiveWriter archive;
    const AssetId scene = makeAssetId("scene");
    const AssetId metadata = makeAssetId("metadata");
    archive.addAsset(scene, AssetType::Scene, 1, std::vector<std::uint32_t>{1, 2, 3});
    archive.addAsset(metadata, AssetType::Texture, 1, std::string{"asset"});

    const auto entries = archive.entries();
    expect(entries.size() == 2, "archive must contain two entries");
    expect(entries[0].offset == 32, "first payload offset must be 32");
    expect(entries[0].size == 16, "vector payload size must be 16");
    expect(entries[1].offset == 48, "second payload offset must be 48");
    expect(entries[1].size == 9, "string payload size must be 9");
    expect(entries[0].crc != 0, "payload checksum must be populated");

    const std::string encoded = encode(archive);
    expect(encoded.size() == 125, "archive size must include header, payloads and directory");
    expect(encoded.compare(0, 4, "ACSA") == 0, "magic must be ACSA");
    expect(readU16(encoded, 4) == kArchiveVersion, "archive version must be 1");
    expect(readU16(encoded, 6) == 0, "reserved header field must be zero");
    expect(readU32(encoded, 8) == 2, "header asset count must be two");
    expect(readU64(encoded, 12) == 57, "directory offset must be 57");
    expect(readU64(encoded, 20) == 68, "directory size must be 68");
    expect(readU32(encoded, 28) == crcForArchive(encoded), "global CRC must match header checksum");

    expect(readU64(encoded, 57) == scene.raw(), "first directory ID must match");
    expect(readU32(encoded, 65) == static_cast<std::uint32_t>(AssetType::Scene), "first directory type must match");
    expect(readU16(encoded, 69) == 1, "first directory schema version must match");
    expect(readU64(encoded, 71) == 32, "first directory payload offset must match");
    expect(readU64(encoded, 79) == 16, "first directory payload size must match");
    expect(readU32(encoded, 87) == entries[0].crc, "first directory payload CRC must match");

    expect(readU64(encoded, 91) == metadata.raw(), "second directory ID must match");
    expect(readU32(encoded, 99) == static_cast<std::uint32_t>(AssetType::Texture), "second directory type must match");
    expect(readU16(encoded, 103) == 1, "second directory schema version must match");
    expect(readU64(encoded, 105) == 48, "second directory payload offset must match");
    expect(readU64(encoded, 113) == 9, "second directory payload size must match");
    expect(readU32(encoded, 121) == entries[1].crc, "second directory payload CRC must match");

    ArchiveWriter empty;
    const std::string emptyArchive = encode(empty);
    expect(empty.entries().empty(), "empty archive must have no entries");
    expect(emptyArchive.size() == kArchiveHeaderSize, "empty archive must contain only the header");
    expect(readU32(emptyArchive, 8) == 0, "empty archive count must be zero");
    expect(readU64(emptyArchive, 12) == kArchiveHeaderSize, "empty archive directory offset must follow header");
    expect(readU64(emptyArchive, 20) == 0, "empty archive directory size must be zero");
    expect(readU32(emptyArchive, 28) == crcForArchive(emptyArchive), "empty archive CRC must be valid");

    ArchiveWriter adapters;
    adapters.addAsset(makeAssetId("material"), AssetType::Texture, 1,
                      Material{"metal \u2605", {0.8f, 0.6f, 0.2f}});
    adapters.addAsset(makeAssetId("library"), AssetType::Mesh, 1,
                      LibraryEntry{"Binary archives", 2026, true});
    const auto adapterEntries = adapters.entries();
    expect(adapterEntries.size() == 2, "custom adapters must create two assets");
    expect(adapterEntries[0].size > 0, "material adapter payload must not be empty");
    expect(adapterEntries[1].size > 0, "library adapter payload must not be empty");
    expect(!encode(adapters).empty(), "custom adapter archive must encode");

    const std::filesystem::path testArchive =
        std::filesystem::temp_directory_path() / "ac-save-writer-test.acsa";
    adapters.save(testArchive);
    std::ifstream saved(testArchive, std::ios::binary);
    expect(saved.good(), "save must create a readable archive file");
    saved.close();
    std::error_code removeError;
    std::filesystem::remove(testArchive, removeError);

    expectFailure([&] {
        adapters.save(std::filesystem::path{"a-directory-that-does-not-exist"} /
                      "archive.acsa");
    });
    expect(!std::filesystem::exists("a-directory-that-does-not-exist"), 
           "failed save must not create a partial destination");

    std::string altered = encoded;
    altered[32] ^= static_cast<char>(1);
    expect(readU32(altered, 28) == crcForArchive(encoded),
           "stored global checksum must remain tied to the original archive");
    expect(crcForArchive(altered) != crcForArchive(encoded),
           "altering a payload byte must change the global CRC");

    const std::string truncated = encoded.substr(0, encoded.size() - 1);
    expect(truncated.size() == 124, "truncated archive fixture must be one byte short");
    expect(truncated.size() < readU64(truncated, 12) + readU64(truncated, 20),
           "truncated archive must end before the directory");

    expectFailure([&] { archive.addAsset(scene, AssetType::Scene, 1, std::uint32_t{4}); });
    expectFailure([&] { archive.addAsset(AssetId{}, AssetType::Scene, 1, std::uint32_t{4}); });
    expectFailure([&] { archive.addAsset(makeAssetId("unknown"), AssetType::Unknown, 1, std::uint32_t{4}); });

    BadOutputStream badOutput;
    expectFailure([&] { archive.writeTo(badOutput); });

    if (failures != 0)
    {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All archive writer tests passed\n";
    return 0;
}
