// LoginInfo2Parser: Documents/LoginInfo2.dat in the backup (protobuf, walked as unknown fields via
// RawMessage) -> the list of WeChat accounts. Mining hint a79de3d8f5 (offset < length loop bound).
#include <vector>
#include "fuzz_common.h"
#include "WechatParser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_write("LoginInfo2.dat", data, size);
    LoginInfo2Parser parser(NULL, NULL);
    std::vector<Friend> users;
    parser.parse(fuzz_path("LoginInfo2.dat"), users);
    for (const Friend& f : users)
    {
        f.getUsrName();
        f.getDisplayName();
    }
    return 0;
}
