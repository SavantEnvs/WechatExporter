// Linux replacement for WechatExporter/core/Utils_md5.cpp, which only has Win32 (CryptoAPI) and
// Apple (CommonCrypto) branches and #errors elsewhere. Same functions, same output (lower-case hex),
// computed with OpenSSL's EVP digests. Platform glue only: no parsing logic lives here.
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <openssl/evp.h>
#include "FileSystem.h"

static std::string digestHex(const EVP_MD* md, const void* data, size_t size)
{
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    EVP_Digest(data, size, out, &len, md, NULL);
    std::stringstream stream;
    stream << std::setfill('0') << std::hex;
    for (unsigned int i = 0; i < len; i++)
    {
        stream << std::setw(2) << ((unsigned int)out[i]);
    }
    return stream.str();
}

std::string md5Impl(const void* data, size_t dataSize)
{
    return digestHex(EVP_md5(), data, dataSize);
}

std::string md5(const std::string& s)
{
    return md5Impl(s.c_str(), s.size());
}

std::string md5File(const std::string& path)
{
    std::vector<unsigned char> data;
    if (readFile(path, data) && !data.empty())
    {
        return md5Impl(&data[0], data.size());
    }
    return "";
}

std::string sha1(const std::string& s)
{
    return digestHex(EVP_sha1(), s.c_str(), s.size());
}
