#include "security/Obfuscation.h"
#include "core/Logger.h"
#include "core/Platform.h"

#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#elif PLATFORM_LINUX
#include <unistd.h>
#include <fcntl.h>
#include <sys/random.h>
#endif

namespace security {

std::string Obfuscation::ObfuscateString(const std::string& input) {
    std::string obfuscated = "OBF:";
    std::vector<uint8_t> buffer(input.begin(), input.end());
    
    // Simple XOR obfuscation with rotating key
    const uint8_t key[] = {0x3A, 0x7F, 0xC2, 0x15};
    size_t keyIndex = 0;
    
    for (auto& byte : buffer) {
        byte ^= key[keyIndex];
        keyIndex = (keyIndex + 1) % sizeof(key);
        
        // Convert to hex representation
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", byte);
        obfuscated += hex;
    }
    
    return obfuscated;
}

std::string Obfuscation::DeobfuscateString(const std::string& input) {
    if (input.find("OBF:") != 0) {
        return input; // Not obfuscated
    }
    
    std::string hexData = input.substr(4);
    std::vector<uint8_t> buffer;
    
    // Convert hex to bytes
    for (size_t i = 0; i < hexData.length(); i += 2) {
        std::string byteString = hexData.substr(i, 2);
        uint8_t byte = (uint8_t)strtoul(byteString.c_str(), NULL, 16);
        buffer.push_back(byte);
    }
    
    // XOR deobfuscation with same rotating key
    const uint8_t key[] = {0x3A, 0x7F, 0xC2, 0x15};
    size_t keyIndex = 0;
    
    for (auto& byte : buffer) {
        byte ^= key[keyIndex];
        keyIndex = (keyIndex + 1) % sizeof(key);
    }
    
    return std::string(buffer.begin(), buffer.end());
}

void Obfuscation::EncryptStringInPlace(std::string& str) {
    const uint32_t key = 0xDEADBEEF;
    uint32_t state = key;
    
    for (char& c : str) {
        c ^= (state & 0xFF);
        state = (state << 1) | (state >> 31); // Rotate left
    }
}

void Obfuscation::DecryptStringInPlace(std::string& str) {
    const uint32_t key = 0xDEADBEEF;
    uint32_t state = key;
    
    for (char& c : str) {
        c ^= (state & 0xFF);
        state = (state << 1) | (state >> 31); // Rotate left
    }
}

std::string Obfuscation::GenerateRandomString(size_t length) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::string randomString;
    randomString.reserve(length);
    
#if PLATFORM_WINDOWS
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
#elif PLATFORM_LINUX
    unsigned char* buffer = new unsigned char[length];
    ssize_t result = getrandom(buffer, length, 0);
    if (result == static_cast<ssize_t>(length)) {
        for (size_t i = 0; i < length; i++) {
            randomString += alphanum[buffer[i] % (sizeof(alphanum) - 1)];
        }
    }
    delete[] buffer;
#endif
    
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

void Obfuscation::ApplyCodeObfuscation() {
    // Insert random garbage instructions that don't affect functionality
    volatile int junk = 0;
    for (int i = 0; i < 10; i++) {
        junk += i * i;
        junk -= i;
    }
    
    // Platform-specific obfuscation techniques
#if PLATFORM_WINDOWS && defined(_MSC_VER) && defined(_M_IX86)
    // Random jumps to disrupt static analysis on Windows (x86 only)
    __asm {
        jmp $+2
        nop
        nop
        jmp $+2
    }
#elif PLATFORM_LINUX && defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    // Linux-specific obfuscation techniques (x86/x64 only)
    asm volatile (
        "jmp 1f\n\t"
        "nop\n\t"
        "1:\n\t"
        "jmp 2f\n\t"
        "nop\n\t"
        "2:\n\t"
        : 
        : 
        : "memory"
    );
#endif
}

std::string Obfuscation::Base64Encode(const std::vector<uint8_t>& data) {
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

std::vector<uint8_t> Obfuscation::Base64Decode(const std::string& encoded) {
    // Use a function to generate the base64 index instead of large static array
    auto getBase64Index = [](uint8_t c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        if (c == '=') return -2; // Padding
        return -1; // Invalid character
    };
    
    std::vector<uint8_t> decoded;
    decoded.reserve((encoded.size() * 3) / 4);
    
    size_t i = 0;
    while (i < encoded.size()) {
        int val1 = getBase64Index(static_cast<uint8_t>(encoded[i++]));
        int val2 = getBase64Index(static_cast<uint8_t>(encoded[i++]));
        int val3 = getBase64Index(static_cast<uint8_t>(encoded[i++]));
        int val4 = getBase64Index(static_cast<uint8_t>(encoded[i++]));
        
        if (val1 < 0 || val2 < 0) break;
        
        uint32_t triple = (val1 << 18) | (val2 << 12) | 
                         ((val3 >= 0) ? val3 << 6 : 0) | 
                         ((val4 >= 0) ? val4 : 0);
        
        decoded.push_back((triple >> 16) & 0xFF);
        if (val3 >= 0) decoded.push_back((triple >> 8) & 0xFF);
        if (val4 >= 0) decoded.push_back(triple & 0xFF);
    }
    
    return decoded;
}

} // namespace security