// XmlParser: the libxml2/XPath wrapper MessageParser runs over message XML stored in the
// backup's databases (appmsg, emoji, forwarded records, system messages).
#include <map>
#include <string>
#include "XmlParser.h"

static const char* kValueXPaths[] = {
    "/msg/appmsg/title", "/msg/appmsg/des", "/msg/appmsg/type", "/msg/appmsg/url",
    "/msg/appmsg/recorditem", "/msg/appmsg/appattach/totallen", "/msg/emoji",
    "/sysmsg/@type", "/msg/videomsg", "/msg/img", "/msg/location", "//title",
};
static const char* kNodesXPaths[] = { "/msg/appmsg/*", "/msg/*", "/sysmsg/*", "/recordinfo/*" };

class FuzzHandler
{
public:
    bool operator()(xmlNodeSetPtr xpathNodes)
    {
        for (int i = 0; xpathNodes && i < xpathNodes->nodeNr; ++i)
        {
            xmlNodePtr n = xpathNodes->nodeTab[i];
            std::string v;
            XmlParser::getNodeInnerText(n);
            XmlParser::getNodeInnerXml(n);
            XmlParser::getNodeOuterXml(n);
            XmlParser::getChildNodeContent(n, "datatitle", v);
            XmlParser::getChildNodeContent(n, "datadesc", v);
            XmlParser::getNodeAttributeValue(n, "datatype", v);
            XmlParser::getChildNode(n, "sourcename");
            XmlParser::getNextNodeSibling(n);
        }
        return true;
    }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string xml(reinterpret_cast<const char*>(data), size);
    XmlParser parser(xml, true);
    std::string value;
    for (const char* xp : kValueXPaths)
    {
        parser.parseNodeValue(xp, value);
    }
    for (const char* xp : kNodesXPaths)
    {
        std::map<std::string, std::string> values;
        parser.parseNodesValue(xp, values);
    }
    std::map<std::string, std::string> attrs;
    parser.parseAttributesValue("/msg/emoji", attrs);
    parser.parseAttributeValue("/msg/img", "length", value);
    parser.parseAttributeValue("/sysmsg", "type", value);

    FuzzHandler handler;
    parser.parseWithHandler("/recordinfo/datalist/dataitem", handler);
    parser.parseWithHandler("/msg/appmsg/*", handler);
    return 0;
}
