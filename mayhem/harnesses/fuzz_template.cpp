// Template: the %%NAME%% placeholder scanner + substitution used for every exported HTML/TXT page
// (templates come from the res/ directory, which users can customise).
#include <map>
#include <string>
#include <vector>
#include "Template.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string tmpl(reinterpret_cast<const char*>(data), size);
    Template t(tmpl);
    std::map<std::string, std::string> values;
    values["%%NAME%%"] = "alice";
    values["%%TEXT%%"] = "hello";
    values["%%TIME%%"] = "2020-01-01";
    values["%%%%"] = "pct";
    t.build(values);
    Template copy = t;
    copy.build(values);
    return 0;
}
