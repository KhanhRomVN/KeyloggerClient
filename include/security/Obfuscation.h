#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Add this macro definition
#define OBFUSCATE(str) (::security::Obfuscation::ObfuscateString(str).c_str())

namespace security {

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
};

} // namespace security