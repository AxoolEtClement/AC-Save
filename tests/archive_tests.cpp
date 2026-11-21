#include "binary_writer/archiveWriter.hpp"

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace AC_Save;

namespace
{
    template <class Function>
    void expectFailure(Function&& function)
    {
        bool failed = false;
        try { function(); } catch (const std::exception&) { failed = true; }
        assert(failed);
    }

    std::string encode(const ArchiveWriter& archive)
    {
        std::ostringstream output(std::ios::out | std::ios::binary);
        archive.writeTo(output);
        return output.str();
    }
}

int main()
{
    ArchiveWriter archive;
    const AssetId scene = makeAssetId("scene");
    const AssetId metadata = makeAssetId("metadata");
    archive.addAsset(scene, AssetType::Scene, 1, std::vector<std::uint32_t>{1, 2, 3});
    archive.addAsset(metadata, AssetType::Texture, 1, std::string{"asset"});

    const auto entries = archive.entries();
    assert(entries.size() == 2);
    assert(entries[0].offset == 32);
    assert(entries[0].size == 16);
    assert(entries[1].offset == 48);
    assert(entries[1].size == 9);
    assert(entries[0].crc != 0);
    assert(!encode(archive).empty());

    ArchiveWriter empty;
    const std::string emptyArchive = encode(empty);
    assert(empty.entries().empty());
    assert(emptyArchive.size() == 32);

    expectFailure([&] { archive.addAsset(scene, AssetType::Scene, 1, std::uint32_t{4}); });
    expectFailure([&] { archive.addAsset(AssetId{}, AssetType::Scene, 1, std::uint32_t{4}); });
    expectFailure([&] { archive.addAsset(makeAssetId("unknown"), AssetType::Unknown, 1, std::uint32_t{4}); });
}
