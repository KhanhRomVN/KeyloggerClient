#include "utils/StringUtils.h"
#include <Windows.h>
#include <wincrypt.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <random>
#include <vector>   
#include <cstdint>
#include <string>
#include <cstdarg>

#pragma comment(lib, "advapi32.lib")

std::string utils::StringUtils::WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), 
                                 NULL, 0, NULL, NULL);
    std::string utf8(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), 
                        &utf8[0], size, NULL, NULL);
    return utf8;
}

std::wstring utils::StringUtils::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), 
                                  NULL, 0);
    std::wstring wide(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), 
                       &wide[0], size);
    return wide;
}

std::string utils::StringUtils::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string utils::StringUtils::ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string utils::StringUtils::Trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        ++start;
    }
    
    auto end = str.end();
    do {
        --end;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    
    return std::basic_string<char>(start, end + 1);
}

std::vector<std::string> utils::StringUtils::Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string utils::StringUtils::Join(const std::vector<std::string>& tokens, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (i > 0) result += delimiter;
        result += tokens[i];
    }
    return result;
}

bool utils::StringUtils::StartsWith(const std::string& str, const std::string& prefix) {
    if (prefix.size() > str.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool utils::StringUtils::EndsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::string utils::StringUtils::Replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

std::string utils::StringUtils::GenerateRandomString(size_t length) {
    static constexpr char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::string randomString;
    randomString.reserve(length);
    
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        BYTE* buffer = new BYTE[length];
        if (CryptGenRandom(hProv, (DWORD)length, buffer)) {
            for (size_t i = 0; i < length; i++) {
                randomString += alphanum[buffer[i] % (sizeof(alphanum) - 1)];
            }
        }
        delete[] buffer;
        CryptReleaseContext(hProv, 0);
    }
    
    // Fallback to std::random if crypto API fails
    if (randomString.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
        
        for (size_t i = 0; i < length; i++) {
            randomString += alphanum[dis(gen)];
        }
    }
    
    return randomString;
}

void utils::StringUtils::GenerateRandomBytes(uint8_t* buffer, size_t length) {
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, (DWORD)length, buffer);
        CryptReleaseContext(hProv, 0);
    } else {
        // Fallback to std::random
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (size_t i = 0; i < length; i++) {
            buffer[i] = static_cast<uint8_t>(dis(gen));
        }
    }
}

std::string utils::StringUtils::Base32Encode(const std::vector<uint8_t>& data) {
    static constexpr char base32Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    
    std::string encoded;
    encoded.reserve(((data.size() * 8) + 4) / 5);
    
    size_t buffer = 0;
    size_t bitsLeft = 0;
    
    for (uint8_t byte : data) {
        buffer = (buffer << 8) | byte;
        bitsLeft += 8;
        
        while (bitsLeft >= 5) {
            bitsLeft -= 5;
            encoded += base32Chars[(buffer >> bitsLeft) & 0x1F];
        }
    }
    
    if (bitsLeft > 0) {
        buffer <<= (5 - bitsLeft);
        encoded += base32Chars[buffer & 0x1F];
    }
    
    return encoded;
}

std::vector<uint8_t> utils::StringUtils::Base32Decode(const std::string& encoded) {
    auto getBase32Index = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a';
        if (c >= '2' && c <= '7') return 26 + (c - '2');
        return -1;
    };
    
    std::vector<uint8_t> decoded;
    decoded.reserve((encoded.size() * 5) / 8);
    
    size_t buffer = 0;
    size_t bitsLeft = 0;
    
    for (char c : encoded) {
        if (c == '=') break;
        
        int value = getBase32Index(c);
        if (value == -1) continue;
        
        buffer = (buffer << 5) | value;
        bitsLeft += 5;
        
        if (bitsLeft >= 8) {
            bitsLeft -= 8;
            decoded.push_back((buffer >> bitsLeft) & 0xFF);
        }
    }
    
    return decoded;
}

std::string utils::StringUtils::Base64Encode(const std::vector<uint8_t>& data) {
    static constexpr char base64Chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string encoded;
    encoded.reserve(((data.size() + 2) / 3) * 4);
    
    size_t i = 0;
    while (i < data.size()) {
        uint32_t octet1 = i < data.size() ? data[i++] : 0;
        uint32_t octet2 = i < data.size() ? data[i++] : 0;
        uint32_t octet3 = i < data.size() ? data[i++] : 0;
        
        uint32_t triple = (octet1 << 16) | (octet2 << 8) | octet3;
        
        encoded += base64Chars[(triple >> 18) & 0x3F];
        encoded += base64Chars[(triple >> 12) & 0x3F];
        encoded += base64Chars[(triple >> 6) & 0x3F];
        encoded += base64Chars[triple & 0x3F];
    }
    
    size_t padding = data.size() % 3;
    if (padding > 0) {
        encoded[encoded.size() - 1] = '=';
        if (padding == 1) {
            encoded[encoded.size() - 2] = '=';
        }
    }
    
    return encoded;
}

std::vector<uint8_t> utils::StringUtils::Base64Decode(const std::string& encoded) {
    auto getBase64Index = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return 26 + (c - 'a');
        if (c >= '0' && c <= '9') return 52 + (c - '0');
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    
    std::vector<uint8_t> decoded;
    decoded.reserve((encoded.size() * 3) / 4);
    
    size_t i = 0;
    while (i < encoded.size()) {
        int val1 = getBase64Index(encoded[i++]);
        int val2 = i < encoded.size() ? getBase64Index(encoded[i++]) : -1;
        int val3 = i < encoded.size() ? getBase64Index(encoded[i++]) : -1;
        int val4 = i < encoded.size() ? getBase64Index(encoded[i++]) : -1;
        
        if (val1 == -1 || val2 == -1) break;
        
        uint32_t triple = (val1 << 18) | (val2 << 12) | 
                         ((val3 != -1) ? val3 << 6 : 0) | 
                         ((val4 != -1) ? val4 : 0);
        
        decoded.push_back((triple >> 16) & 0xFF);
        if (val3 != -1) decoded.push_back((triple >> 8) & 0xFF);
        if (val4 != -1) decoded.push_back(triple & 0xFF);
    }
    
    return decoded;
}

std::string utils::StringUtils::Format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);
    
    if (size <= 0) {
        va_end(args);
        return "";
    }
    
    std::string result(size, 0);
    vsnprintf(&result[0], size + 1, format, args);
    va_end(args);
    
    return result;
}