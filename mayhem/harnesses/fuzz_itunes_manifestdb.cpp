// ITunesDb over an iOS 10+ backup: Manifest.db (sqlite, Files table) plus the per-file bplist blob
// (ITunesDb::parseFileInfo / parseModifiedTime). Mining hint a700233d3c (empty blob).
#include <memory>
#include <vector>
#include "fuzz_common.h"
#include "ITunesParser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_mkdirs("mdb");
    fuzz_write("mdb/Manifest.db", data, size);
    {
        ITunesDb db(fuzz_path("mdb"), "Manifest.db");
        if (db.load("AppDomain-com.tencent.xin", false))
        {
            const ITunesFile* f = db.findITunesFile("Documents/LoginInfo2.dat");
            if (f) ITunesDb::parseFileInfo(f);
        }
        std::vector<std::string> domains;
        std::unique_ptr<ITunesDb::ITunesFileEnumerator> e(db.buildEnumerator(domains, false));
        if (!e->isInvalid())
        {
            ITunesFile f;
            int n = 0;
            while (n++ < 4096 && e->nextFile(f))
            {
                ITunesDb::parseFileInfo(&f);
            }
        }
    }
    fuzz_remove("mdb/Manifest.db-journal");
    fuzz_remove("mdb/Manifest.db-wal");
    fuzz_remove("mdb/Manifest.db-shm");
    return 0;
}
