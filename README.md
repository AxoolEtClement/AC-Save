# AC-Save — Asset Engine

Small C++23 binary asset archive used as the reusable core of the Asset Engine
project. The archive code is independent from a physics engine and accepts
domain types through the existing `Serializer<T>` adapter specialization.
This repository currently owns the writer side only; the reader is implemented
by the other project contributor.

## Build and verification

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/AC-saveDemo   # creates demo.acsa
```

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
`ostream` or atomically save to a filesystem path. The reader and validation
of an existing archive are intentionally outside this contribution.

Strings and sequences use a 32-bit element/byte count. Values are encoded
field-by-field rather than by dumping C++ object memory, so padding, pointers
and container internals never become persistent data. Version migration is
represented by the `schemaVersion` entry field; adapters should provide
defaults when reading an older schema.

## Scope and limitations

The current tests validate serialization of strings, sequences and multiple
assets, deterministic offsets and CRC generation, empty archives, invalid IDs,
unknown types and duplicate IDs. Physics scene/state adapters, schema
migration readers, corruption/truncation rejection and performance
measurements remain project-level work for the complete pair submission.
CRC-32 is an integrity check only; archives are not encrypted or authenticated.

## Usage

```cpp
#include "binary_writer/archiveWriter.hpp"

struct Material { std::string name; std::vector<float> values; };

template <>
struct AC_Save::Serializer<Material>
{
    static void write(AC_Save::BinaryWriter& writer, const Material& value)
    {
        AC_Save::write(writer, value.name);
        AC_Save::write(writer, value.values);
    }
};

int main()
{
    AC_Save::ArchiveWriter archive;
    archive.addAsset(AC_Save::makeAssetId("material"),
                     AC_Save::AssetType::Texture, 1,
                     Material{"metal", {0.8f, 0.6f}});
    archive.save("assets.acsa");
}
```

`addAsset` serializes field-by-field, rejects ID zero, unknown types and
duplicates, and stores the schema version in the directory. Use `writeTo` when
the caller owns an output stream; use `save` for atomic temporary-file writing.

AI assistance was used for code cleanup and test design. All generated changes
were compiled and verified with the test command above.
