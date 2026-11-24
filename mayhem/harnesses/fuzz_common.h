// Shared helpers for the file-based WechatExporter harnesses: a private scratch directory
// (created once under $TMPDIR, contents rewritten for every input) so each input's result
// depends only on that input.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

static inline const std::string& fuzz_scratch_dir()
{
    static std::string dir;
    if (dir.empty())
    {
        const char* t = getenv("TMPDIR");
        std::string tmpl = std::string((t && *t) ? t : "/tmp") + "/wefuzz-XXXXXX";
        char buf[4096];
        snprintf(buf, sizeof(buf), "%s", tmpl.c_str());
        if (mkdtemp(buf) == NULL) abort();
        dir = buf;
    }
    return dir;
}

static inline std::string fuzz_path(const std::string& rel)
{
    return fuzz_scratch_dir() + "/" + rel;
}

static inline void fuzz_mkdirs(const std::string& rel)
{
    std::string full = fuzz_scratch_dir();
    size_t start = 0;
    while (start < rel.size())
    {
        size_t end = rel.find('/', start);
        if (end == std::string::npos) end = rel.size();
        full += "/" + rel.substr(start, end - start);
        mkdir(full.c_str(), 0700);
        start = end + 1;
    }
}

static inline void fuzz_write(const std::string& rel, const uint8_t* data, size_t size)
{
    FILE* f = fopen(fuzz_path(rel).c_str(), "wb");
    if (!f) abort();
    if (size) fwrite(data, 1, size, f);
    fclose(f);
}

static inline void fuzz_remove(const std::string& rel)
{
    unlink(fuzz_path(rel).c_str());
}
