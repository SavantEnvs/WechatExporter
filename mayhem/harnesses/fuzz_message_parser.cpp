// MessageParser::parse over one chat-message record (WXMSG.content), the XML/regex parsing of
// message bodies that a WeChat backup's Message.sqlite carries verbatim from the sender:
// share cards (42/66), emoticons (47), locations (48), app messages (49, incl. forwarded-message
// records, channels, refer/quote, pat, transfers), calls, notices and system messages.
//
// Input layout: byte 0 selects the message type from kTypes (the types MessageParser::parse
// dispatches on), byte 1 bit 0 selects a group-chat session (so "<sender>:\n" prefixes are split
// off as in a real chatroom), bit 1 sets msg.des (incoming vs. outgoing); the rest is the content.
//
// ExportOption is in text mode, the way WechatExporter's text export runs: in that mode every
// TaskManager::download and ITunesDb::copyFile call site in MessageParser is gated off (portrait,
// card avatar, channels thumb/avatar, emoji, image/video/file copies, voice transcoding), so the
// real TaskManager below never receives a task and no file is read from a backup.
//
// State kept across inputs, on purpose: the read-only ResManager (zh-Hans strings + text
// templates loaded from the source tree's WechatExporter/res), the fixed Friends/Session objects
// (MessageParser only looks them up), an empty ITunesDb, and the idle TaskManager. Files the
// parser writes go to this process's private scratch directory: the `Assets/Portrait` directories
// it creates and, in a !NDEBUG build, the dbg/msg_*.txt dumps; none of them is read back.
#include <limits.h>
#include <unistd.h>
#include <memory>
#include <vector>
#include "fuzz_common.h"
#include "MessageParser.h"
#include "Logger.h"

namespace {

struct NullLogger : public Logger
{
    void write(const std::string&) override {}
    void debug(const std::string&) override {}
};

const int kTypes[] = {
    MessageParser::MSGTYPE_TEXT, MessageParser::MSGTYPE_IMAGE, MessageParser::MSGTYPE_VOICE,
    MessageParser::MSGTYPE_PUSHMAIL, MessageParser::MSGTYPE_VERIFYMSG, MessageParser::MSGTYPE_POSSIBLEFRIEND,
    MessageParser::MSGTYPE_SHARECARD, MessageParser::MSGTYPE_VIDEO, MessageParser::MSGTYPE_EMOTICON,
    MessageParser::MSGTYPE_LOCATION, MessageParser::MSGTYPE_APP, MessageParser::MSGTYPE_VOIPMSG,
    MessageParser::MSGTYPE_STATUSNOTIFY, MessageParser::MSGTYPE_VOIPNOTIFY, MessageParser::MSGTYPE_VOIPINVITE,
    MessageParser::MSGTYPE_MICROVIDEO, MessageParser::MSGTYPE_NOTICE, MessageParser::MSGTYPE_IMCARD,
    MessageParser::MSGTYPE_SYSNOTICE, MessageParser::MSGTYPE_SYS, MessageParser::MSGTYPE_RECALLED,
};

// The source tree's WechatExporter/ directory (holds res/), found from this binary's location
// (<src>/mayhem/build/<binary>) so it works wherever the tree is.
std::string resRoot()
{
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) abort();
    buf[n] = 0;
    std::string p(buf);
    p = p.substr(0, p.rfind('/'));          // .../mayhem/build
    p = p.substr(0, p.rfind('/'));          // .../mayhem
    p = p.substr(0, p.rfind('/'));          // <src>
    return p + "/WechatExporter";
}

struct Fixture
{
    NullLogger logger;
    ResManager res;
    TaskManager taskManager;
    std::unique_ptr<ITunesDb> db;
    Friends friends;
    Friend myself;
    std::unique_ptr<Session> single;
    std::unique_ptr<Session> chatroom;
    std::unique_ptr<MessageParser> parser;

    Fixture() : taskManager(&logger)
    {
        std::string root = resRoot();
        // The resources must be there (a missing res/ would silently turn every message into an
        // empty locale lookup). initResources itself reports false for templates_txt because that
        // directory has no filter/member templates (HTML-only); everything present is loaded.
        if (!res.initLocaleResource(root, "zh-Hans")) abort();
        res.initResources(root, "zh-Hans", "templates_txt");

        fuzz_mkdirs("mp/out");
        db.reset(new ITunesDb(fuzz_path("mp/backup"), "Manifest.db"));   // never loaded: no files

        myself = Friend("wxid_me", md5("wxid_me"));
        myself.setDisplayName("Me");
        Friend& alice = friends.addFriend("wxid_alice");
        alice.setDisplayName("Alice");
        Friend& bob = friends.addFriend("wxid_bob");
        bob.setDisplayName("Bob");

        single.reset(new Session("wxid_alice", md5("wxid_alice"), &myself));
        single->setDisplayName("Alice");
        single->setOutputFileName("alice");
        chatroom.reset(new Session("123456@chatroom", md5("123456@chatroom"), &myself));
        chatroom->setDisplayName("Group");
        chatroom->setOutputFileName("group");

        ExportOption options;
        options.setTextMode();
        parser.reset(new MessageParser(*db, *db, taskManager, friends, myself, options,
                                       root, fuzz_path("mp/out"), res));
    }
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    static Fixture* fx = new Fixture();
    if (size < 2) return -1;

    WXMSG msg;
    msg.createTime = 1600000000;
    msg.type = kTypes[data[0] % (sizeof(kTypes) / sizeof(kTypes[0]))];
    msg.des = (data[1] & 2) ? 1 : 0;
    msg.msgId = "1001";
    msg.msgIdValue = 1001;
    msg.msgSvrId = 1001;
    msg.status = 0;
    msg.tableVersion = 0;
    msg.content.assign(reinterpret_cast<const char*>(data + 2), size - 2);

    const Session& session = (data[1] & 1) ? *fx->chatroom : *fx->single;
    std::vector<TemplateValues> tvs;
    fx->parser->parse(msg, session, tvs);
    return 0;
}
