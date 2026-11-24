// Known-answer functional checks for WechatExporter's portable core (upstream has no Linux test
// suite; its only test is an Objective-C++ XCTest). Each check prints "PASS <name>" or
// "FAIL <name>: got ..." and the process exits non-zero if any check fails.
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>
#include <sqlite3.h>
#include "Template.h"
#include "XmlParser.h"
#include "Utils.h"
#include "ITunesParser.h"

static int g_pass = 0, g_fail = 0;

static void check(const char* name, const std::string& got, const std::string& want)
{
    if (got == want) { printf("PASS %s\n", name); g_pass++; }
    else { printf("FAIL %s: got [%s] want [%s]\n", name, got.c_str(), want.c_str()); g_fail++; }
}

int main()
{
    // Template substitution: known tags replaced, unknown tags erased.
    {
        Template t("<p>%%NAME%% says %%TEXT%%%%MISSING%%</p>");
        std::map<std::string, std::string> v;
        v["%%NAME%%"] = "alice"; v["%%TEXT%%"] = "hi";
        check("template_build", t.build(v), "<p>alice says hi</p>");
    }
    // XmlParser XPath extraction over appmsg XML.
    {
        XmlParser p("<msg><appmsg appid=\"wx1\"><title>Hello</title><des><![CDATA[World]]></des>"
                    "<type>5</type></appmsg></msg>", true);
        std::string v;
        p.parseNodeValue("/msg/appmsg/title", v); check("xml_title", v, "Hello");
        v.clear(); p.parseNodeValue("/msg/appmsg/des", v); check("xml_cdata", v, "World");
        v.clear(); p.parseAttributeValue("/msg/appmsg", "appid", v); check("xml_attr", v, "wx1");
        std::map<std::string, std::string> m;
        m["title"] = ""; m["type"] = "";
        p.parseNodesValue("/msg/appmsg/*", m);
        check("xml_nodes", m["title"] + "|" + m["type"], "Hello|5");
    }
    // String helpers used when rendering messages.
    check("safe_html", safeHTML("a<b>&c\n"), "a&lt;b&gt;&amp;c<br/>");
    check("remove_cdata", removeCdata("<![CDATA[x y]]>"), "x y");
    check("decode_url", decodeUrl("a%20b%2Fc"), "a b/c");
    check("md5", md5("abc"), "900150983cd24fb0d6963f7d28e17f72");
    // ITunesDb over a Manifest.db with two rows in the Files table.
    {
        const char* t = getenv("TMPDIR");
        std::string base = std::string((t && *t) ? t : "/tmp") + "/weselftest-XXXXXX";
        std::vector<char> buf(base.begin(), base.end()); buf.push_back(0);
        std::string dir = mkdtemp(buf.data()) ? buf.data() : "";
        std::string db = dir + "/Manifest.db";
        sqlite3* h = NULL;
        sqlite3_open(db.c_str(), &h);
        sqlite3_exec(h, "CREATE TABLE Files (fileID TEXT PRIMARY KEY, domain TEXT, relativePath TEXT, flags INTEGER, file BLOB);"
                        "INSERT INTO Files VALUES('aabbccddeeff00112233445566778899aabbccdd','AppDomain-com.tencent.xin','Documents/LoginInfo2.dat',1,NULL);"
                        "INSERT INTO Files VALUES('1111111111111111111111111111111111111111','AppDomain-com.tencent.xin','Documents',2,NULL);"
                        "INSERT INTO Files VALUES('2222222222222222222222222222222222222222','HomeDomain','Library/x.plist',1,NULL);",
                     NULL, NULL, NULL);
        sqlite3_close(h);
        ITunesDb idb(dir, "Manifest.db");
        bool ok = idb.load("AppDomain-com.tencent.xin", true);
        check("itunes_load", ok ? "1" : "0", "1");
        check("itunes_fileid", idb.findFileId("Documents/LoginInfo2.dat"), "aabbccddeeff00112233445566778899aabbccdd");
        check("itunes_domain_filter", idb.findFileId("Library/x.plist"), "");
        unlink(db.c_str());
        rmdir(dir.c_str());
    }
    printf("SELFTEST passed=%d failed=%d\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
