// MMKVParser: the mmsetting.archive.<user> MMKV store and its .crc companion from the backup.
// Input layout: the first 36 bytes (if present) are the .crc file, the rest is the archive.
#include "fuzz_common.h"
#include "WechatParser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    size_t crcLen = size < 36 ? size : 36;
    fuzz_write("mmsetting.archive.crc", data, crcLen);
    fuzz_write("mmsetting.archive", data + crcLen, size - crcLen);
    MMKVParser parser(NULL);
    if (parser.parse(fuzz_path("mmsetting.archive"), fuzz_path("mmsetting.archive.crc")))
    {
        parser.getUsrName();
        parser.getWxName();
        parser.getDisplayName();
        parser.getPortrait();
        parser.getPortraitHD();
    }
    return 0;
}
