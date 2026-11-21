#include "binary_writer/archiveWriter.hpp"

#include <iostream>
#include <string>

int main()
{
    AC_Save::ArchiveWriter archive;
    archive.addAsset(AC_Save::makeAssetId("demo"), AC_Save::AssetType::Scene, 1,
                     std::string{"Asset Engine"});

    archive.save("demo.acsa");
    std::cout << "Wrote " << archive.entries().size() << " asset(s) to demo.acsa\n";
}
