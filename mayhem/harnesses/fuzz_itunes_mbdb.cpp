// ITunesDb over a pre-iOS 10 backup: Manifest.mbdb (big-endian, length-prefixed records) is the
// index of every file in the backup.
#include <memory>
#include <vector>
#include "fuzz_common.h"
#include "ITunesParser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_mkdirs("mbdb");
    fuzz_write("mbdb/Manifest.mbdb", data, size);
    ITunesDb db(fuzz_path("mbdb"), "Manifest.mbdb");
    if (db.load())
    {
        db.findITunesFile("Documents/LoginInfo2.dat");
        db.findRealPath("Documents/MMappedKV/mmsetting.archive");
        db.findFileId("Library/WechatPrivate/wx.dat");
    }
    std::vector<std::string> domains;
    domains.push_back("AppDomain-com.tencent.xin");
    std::unique_ptr<ITunesDb::ITunesFileEnumerator> e(db.buildEnumerator(domains, false));
    if (!e->isInvalid())
    {
        ITunesFile f;
        int n = 0;
        while (n++ < 4096 && e->nextFile(f)) {}
    }
    return 0;
}
