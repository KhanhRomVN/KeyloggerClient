#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <vector>
#include <string>

class Encryption {
public:
    static std::vector<uint8_t> EncryptAES(const std::vector<uint8_t>& data, const std::string& key);
    static std::vector<uint8_t> DecryptAES(const std::vector<uint8_t>& encryptedData, const std::string& key);
    static std::string GenerateSHA256(const std::string& input);
    static std::string GenerateRandomKey(size_t length);
    static std::vector<uint8_t> XOREncrypt(const std::vector<uint8_t>& data, const std::string& key);
    static std::vector<uint8_t> XORDecrypt(const std::vector<uint8_t>& encryptedData, const std::string& key);
};

#endif