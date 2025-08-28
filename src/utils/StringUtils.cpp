// src/utils/StringUtils.cpp
#include "utils/StringUtils.h"
#include "core/Logger.h"
#include <Windows.h>    
#include <algorithm>
#include <cctype>
#include <sstream>
#include <random>
#include <wincrypt.h>
#include <vector>
#include <cstdint>
#include <string>

#pragma comment(lib, "advapi32.lib")

std::string StringUtils::WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), 
                                 NULL, 0, NULL, NULL);
    std::string utf8(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), 
                        &utf8[0], size, NULL, NULL);
    return utf8;
}

std::wstring StringUtils::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), 
                                  NULL, 0);
    std::wstring wide(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), 
                       &wide[0], size);
    return wide;
}

std::string StringUtils::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string StringUtils::ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string StringUtils::Trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }
    
    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    
    return std::string(start, end + 1);
}

std::vector<std::string> StringUtils::Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string StringUtils::Join(const std::vector<std::string>& tokens, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (i > 0) result += delimiter;
        result += tokens[i];
    }
    return result;
}

bool StringUtils::StartsWith(const std::string& str, const std::string& prefix) {
    if (prefix.size() > str.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool StringUtils::EndsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::string StringUtils::Replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

std::string StringUtils::GenerateRandomString(size_t length) {
    static const char alphanum[] =
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

void StringUtils::GenerateRandomBytes(uint8_t* buffer, size_t length) {
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

std::string StringUtils::Base32Encode(const std::vector<uint8_t>& data) {
    static const char base32Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    
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
    
    // Add padding if needed (typically not for DNS exfiltration)
    // while (encoded.size() % 8 != 0) encoded += '=';
    
    return encoded;
}

std::vector<uint8_t> StringUtils::Base32Decode(const std::string& encoded) {
    static const int base32Index[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,26,27,28,
        29,30,31,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    std::vector<uint8_t> decoded;
    decoded.reserve((encoded.size() * 5) / 8);
    
    size_t buffer = 0;
    size_t bitsLeft = 0;
    
    for (char c : encoded) {
        if (c == '=') break; // Padding
        
        int value = base32Index[(uint8_t)c];
        if (value == -1) continue; // Skip invalid characters
        
        buffer = (buffer << 5) | value;
        bitsLeft += 5;
        
        if (bitsLeft >= 8) {
            bitsLeft -= 8;
            decoded.push_back((buffer >> bitsLeft) & 0xFF);
        }
    }
    
    return decoded;
}

std::string StringUtils::Base64Encode(const std::vector<uint8_t>& data) {
    static const char base64Chars[] =
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
    
    // Add padding
    size_t padding = data.size() % 3;
    if (padding > 0) {
        encoded[encoded.size() - 1] = '=';
        if (padding == 1) {
            encoded[encoded.size() - 2] = '=';
        }
    }
    
    return encoded;
}

std::vector<uint8_t> StringUtils::Base64Decode(const std::string& encoded) {
    static const int base64Index[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,
        56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
        16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,
        36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    std::vector<uint8_t> decoded;
    decoded.reserve((encoded.size() * 3) / 4);
    
    size_t i = 0;
    while (i < encoded.size()) {
        int val1 = base64Index[(uint8_t)encoded[i++]];
        int val2 = base64Index[(uint8_t)encoded[i++]];
        int val3 = base64Index[(uint8_t)encoded[i++]];
        int val4 = base64Index[(uint8_t)encoded[i++]];
        
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

std::string StringUtils::Format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int size = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    
    if (size <= 0) return "";
    
    std::string result(size, 0);
    va_start(args, format);
    vsnprintf(&result[0], size + 1, format, args);
    va_end(args);
    
    return result;
}