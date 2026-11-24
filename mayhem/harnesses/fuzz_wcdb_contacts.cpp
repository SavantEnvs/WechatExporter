// FriendsParser::parseWcdb: the account's DB/WCDB_Contact.sqlite from the backup (Friend and
// OpenIMContact tables; remark / chatroom / head-image columns are protobuf blobs walked by
// RawMessage). Mining hint 17b2528cf7 (NULL sqlite3_column_text handling in the WCDB readers).
#include "fuzz_common.h"
#include "WechatParser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_write("WCDB_Contact.sqlite", data, size);
    {
        FriendsParser parser(true);
        Friends friends;
        parser.parseWcdb(fuzz_path("WCDB_Contact.sqlite"), friends);
        for (auto& it : friends.friends)
        {
            it.second.getUsrName();
            it.second.getDisplayName();
        }
    }
    fuzz_remove("WCDB_Contact.sqlite-journal");
    fuzz_remove("WCDB_Contact.sqlite-wal");
    fuzz_remove("WCDB_Contact.sqlite-shm");
    return 0;
}
