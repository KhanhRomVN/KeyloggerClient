#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstdarg>

namespace utils {
class StringUtils {
public:
    static std::string WideToUtf8(const std::wstring& wide);
    static std::wstring Utf8ToWide(const std::string& utf8);
    
    static std::string ToLower(const std::string& str);
    static std::string ToUpper(const std::string& str);
    static std::string Trim(const std::string& str);
    
    static std::vector<std::string> Split(const std::string& str, char delimiter);
    static std::string Join(const std::vector<std::string>& tokens, const std::string& delimiter);
    
    static bool StartsWith(const std::string& str, const std::string& prefix);
    static bool EndsWith(const std::string& str, const std::string& suffix);
    static std::string Replace(const std::string& str, const std::string& from, const std::string& to);
    
    static std::string GenerateRandomString(size_t length);
    static void GenerateRandomBytes(uint8_t* buffer, size_t length);

    static std::string Base32Encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> Base32Decode(const std::string& encoded);
    
    static std::string Base64Encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> Base64Decode(const std::string& encoded);
    
    static std::string Format(const char* format, ...);
};
}

#endif