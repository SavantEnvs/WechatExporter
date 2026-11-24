// ManifestParser: a backup folder's Info.plist (libplist; device/iTunes fields and the embedded
// per-app iTunesMetadata plists). Manifest.plist / Manifest.db are empty stubs so the folder is
// recognised as a backup item; Manifest.plist is a fixed, valid stub (an empty one would trip an
// unrelated UBSan report in ManifestParser::parse on every input, before Info.plist matters).
#include <vector>
#include "fuzz_common.h"
#include "ITunesParser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_mkdirs("Backup/0000000000000000000000000000000000000000");
    const std::string item = "Backup/0000000000000000000000000000000000000000";
    fuzz_write(item + "/Info.plist", data, size);
    static const char kManifest[] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<plist version=\"1.0\"><dict>"
        "<key>IsEncrypted</key><false/><key>Lockdown</key><dict><key>ProductVersion</key>"
        "<string>14.4</string></dict></dict></plist>\n";
    fuzz_write(item + "/Manifest.plist", reinterpret_cast<const uint8_t*>(kManifest), sizeof(kManifest) - 1);
    fuzz_write(item + "/Manifest.db", NULL, 0);
    ManifestParser parser(fuzz_path(item), true);
    std::vector<BackupItem> items;
    parser.parse(items);
    for (const BackupItem& b : items)
    {
        b.getITunesVersion();
        b.getIOSVersion();
    }
    return 0;
}
