#ifndef OBFUSCATION_H
#define OBFUSCATION_H

#include <string>   
#include <vector>
#include <algorithm>
#include <cstdint>

namespace security {

// Simple XOR-based obfuscation macro
#define OBFUSCATE(str) ([]() -> std::string { \
    constexpr char key = 0x55; \
    std::string result = str; \
    for (size_t i = 0; i < result.size(); ++i) { \
        result[i] ^= key; \
    } \
    return result; \
})()

class Obfuscation {
public:
    static std::string ObfuscateString(const std::string& input);
    static std::string DeobfuscateString(const std::string& input);
    static void EncryptStringInPlace(std::string& str);
    static void DecryptStringInPlace(std::string& str);
    static std::string GenerateRandomString(size_t length);
    static void ApplyCodeObfuscation();
    static std::string Base64Encode(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> Base64Decode(const std::string& encoded);
    
    // Helper method to deobfuscate at runtime
    static std::string RuntimeDeobfuscate(const std::string& obfuscated) {
        std::string result = obfuscated;
        const char key = 0x55;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= key;
        }
        return result;
    }
};

} // namespace security

#endif