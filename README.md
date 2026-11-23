# AC-Save — Asset Engine

Small C++23 binary asset archive used as the reusable core of the Asset Engine
project. The archive code is independent of a physics engine and accepts
domain types through the existing `Serializer<T>` and `Deserializer<T>` adapter specialization.


## Build and verification

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/AC-saveDemo   # creates demo.acsa
```

The demo creates `general-purpose.acsa` and two archives,
`material-v1.acsa` and `material-v2.acsa`, to demonstrate a reusable adapter
and two schema versions.

## Binary format

All integers are fixed-width little-endian values. The 32-byte header contains
the ASCII magic `ACSA`, archive version `1`, an asset count, the absolute
directory offset and size, and a CRC-32. The CRC-32 uses the reflected
polynomial `0xEDB88320` and covers every archive byte, including metadata,
with the checksum field set to zero while calculating it. It detects accidental
corruption but does not provide authenticity.

Payloads are stored consecutively after the header. Each 34-byte directory
entry contains an 8-byte asset ID, 4-byte type, 2-byte schema version,
8-byte absolute payload offset, 8-byte payload size and a CRC-32 of that
payload. The writer exposes `entries()` for inspection and can write to an
`ostream` or atomically save to a filesystem path.

Strings and sequences use a 32-bit element/byte count. Values are encoded
field-by-field rather than by dumping C++ object memory, so padding, pointers
and container internals never become persistent data. Version migration is
represented by the `schemaVersion` entry field; adapters should provide
defaults when reading an older schema.

The demo contains two adapters:

- `Material`: a UTF-8 name and a collection of floating-point coefficients. And an added field float "roughness" for different schemaVersion testing
- `LibraryEntry`: a UTF-8 title, a year and a boolean.

Their `Serializer<T>` specializations demonstrate adding domain types without
modifying the archive core or the binary writer.
Their `Deserializr<T>` specializations have the same purpose with added purpose of showing how to properly handle different schemaVersion if need be.

## Scope and limitations

CRC-32 is an integrity check only; archives are not encrypted or authenticated.

## Usage

```cpp
#include "writer/archiveWriter.hpp"
#include "reader/archiveReader.hpp"

struct Material 
{ 
    std::string name; 
    std::vector<float> values;
    float roughness = 0.8f; 
};

template <>
struct AC_Save::Serializer<Material>
{
    static void write(AC_Save::BinaryWriter& writer, const Material& value)
    {
        AC_Save::write(writer, value.name);
        AC_Save::write(writer, value.values);
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
}

int main()
{
    constexpr auto materialId = AC_Save::makeAssetId("material");
    // writing archive example
    AC_Save::ArchiveWriter archive;
    archive.addAsset(materialId,
                     AC_Save::AssetType::Texture, 1,
                     Material{"metal", {0.8f, 0.6f}});
    archive.save("assets.acsa");
    
    // reading archive example
    AC_Save::ArchiveReader reader("assets.acsa");
    
    reader.readAsset<Material>
    if (reader.findEntry(materialId))
        mat = reader.readAsset<Material>(materialId);
    
}
```

`addAsset` serializes field-by-field, rejects ID zero, unknown types and
duplicates, and stores the schema version in the directory. Use `writeTo` when
the caller owns an output stream; use `save` for atomic temporary-file writing.

to write assets, the serializer must be specialized manually for the specified type.

To read we initialize the reader with the path of the archive file, 
we can also call `ArchiveReader::loadFromBytes(Bytes buffer)` if file has been parsed outside archiveReader 
or even`ArchiveReader::open(const std::filesystem::path& path)` to load a new archive inside already existing reader.

To read assets, we call `ArchiveReader::readAsset<*type*>(*assetId*)`, the deserializer template must be specialized manually for the 
specified type and if the materialId provided doesn't exist in the archive, the functions throws an exception.
`ArchiveReader::findEntry(*assetId*)` gives asset data (id, type, schemaVersion, offset, size, crc)

AI assistance was used for code cleanup and test design. All generated changes
were compiled and verified with the test command above.
