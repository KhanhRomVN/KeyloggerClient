#include "security/Encryption.h"
#include "core/Logger.h"
#include "utils/StringUtils.h"
#include <Windows.h>
#include <wincrypt.h>
#include <vector>
#include <cstdint>
#include <string>
#include <random>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

// Obfuscated strings
constexpr auto OBF_ENCRYPTION = OBFUSCATE("Encryption");
constexpr auto OBF_CRYPTO_ERROR = OBFUSCATE("Crypto error: %s, code: %d");

std::vector<uint8_t> Encryption::EncryptAES(const std::vector<uint8_t>& data, const std::string& key) {
    HCRYPTPROV hProv;
    HCRYPTKEY hKey;
    HCRYPTHASH hHash;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        LOG_ERROR("CryptAcquireContext failed: " + std::to_string(GetLastError()));
        return {};
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        LOG_ERROR("CryptCreateHash failed: " + std::to_string(GetLastError()));
        CryptReleaseContext(hProv, 0);
        return {};
    }

    if (!CryptHashData(hHash, (const BYTE*)key.c_str(), (DWORD)key.length(), 0)) {
        LOG_ERROR("CryptHashData failed: " + std::to_string(GetLastError()));
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, CRYPT_EXPORTABLE, &hKey)) {
        LOG_ERROR("CryptDeriveKey failed: " + std::to_string(GetLastError()));
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    DWORD dwMode = CRYPT_MODE_CBC;
    if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0)) {
        LOG_ERROR("CryptSetKeyParam failed: " + std::to_string(GetLastError()));
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    // Generate random IV
    std::vector<uint8_t> iv(16);
    utils::StringUtils::GenerateRandomBytes(iv.data(), 16);
    if (!CryptSetKeyParam(hKey, KP_IV, iv.data(), 0)) {
        LOG_ERROR("CryptSetKeyParam IV failed: " + std::to_string(GetLastError()));
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    DWORD dwDataLen = (DWORD)data.size();
    DWORD dwBufLen = dwDataLen + 16; // AES block size padding
    std::vector<uint8_t> encryptedData(dwBufLen);

    memcpy(encryptedData.data(), data.data(), dwDataLen);

    if (!CryptEncrypt(hKey, 0, TRUE, 0, encryptedData.data(), &dwDataLen, dwBufLen)) {
        LOG_ERROR("CryptEncrypt failed: " + std::to_string(GetLastError()));
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    // Prepend IV to encrypted data
    encryptedData.insert(encryptedData.begin(), iv.begin(), iv.end());
    encryptedData.resize(iv.size() + dwDataLen);

    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return encryptedData;
}

std::vector<uint8_t> Encryption::DecryptAES(const std::vector<uint8_t>& encryptedData, const std::string& key) {
    if (encryptedData.size() <= 16) {
        LOG_ERROR("Invalid encrypted data size");
        return {};
    }

    HCRYPTPROV hProv;
    HCRYPTKEY hKey;
    HCRYPTHASH hHash;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        LOG_ERROR("CryptAcquireContext failed: " + std::to_string(GetLastError()));
        return {};
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        LOG_ERROR("CryptCreateHash failed: " + std::to_string(GetLastError()));
        CryptReleaseContext(hProv, 0);
        return {};
    }

    if (!CryptHashData(hHash, (const BYTE*)key.c_str(), (DWORD)key.length(), 0)) {
        LOG_ERROR("CryptHashData failed: " + std::to_string(GetLastError()));
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, CRYPT_EXPORTABLE, &hKey)) {
        LOG_ERROR("CryptDeriveKey failed: " + std::to_string(GetLastError()));
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    DWORD dwMode = CRYPT_MODE_CBC;
    if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0)) {
        LOG_ERROR("CryptSetKeyParam failed: " + std::to_string(GetLastError()));
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    // Extract IV from beginning of encrypted data
    std::vector<uint8_t> iv(encryptedData.begin(), encryptedData.begin() + 16);
    if (!CryptSetKeyParam(hKey, KP_IV, iv.data(), 0)) {
        LOG_ERROR("CryptSetKeyParam IV failed: " + std::to_string(GetLastError()));
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    std::vector<uint8_t> decryptedData(encryptedData.begin() + 16, encryptedData.end());
    DWORD dwDataLen = (DWORD)decryptedData.size();

    if (!CryptDecrypt(hKey, 0, TRUE, 0, decryptedData.data(), &dwDataLen)) {
        LOG_ERROR("CryptDecrypt failed: " + std::to_string(GetLastError()));
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    decryptedData.resize(dwDataLen);

    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return decryptedData;
}

std::string Encryption::GenerateSHA256(const std::string& input) {
    HCRYPTPROV hProv;
    HCRYPTHASH hHash;
    BYTE rgbHash[32];
    DWORD cbHash = 32;
    CHAR rgbDigits[] = "0123456789abcdef";

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        LOG_ERROR("CryptAcquireContext failed: " + std::to_string(GetLastError()));
        return "";
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        LOG_ERROR("CryptCreateHash failed: " + std::to_string(GetLastError()));
        CryptReleaseContext(hProv, 0);
        return "";
    }

    if (!CryptHashData(hHash, (const BYTE*)input.c_str(), (DWORD)input.length(), 0)) {
        LOG_ERROR("CryptHashData failed: " + std::to_string(GetLastError()));
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        LOG_ERROR("CryptGetHashParam failed: " + std::to_string(GetLastError()));
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }

    std::string hash;
    for (DWORD i = 0; i < cbHash; i++) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", rgbHash[i]);
        hash += buf;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return hash;
}

std::string Encryption::GenerateRandomKey(size_t length) {
    HCRYPTPROV hProv;
    std::vector<BYTE> buffer(length);

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        LOG_ERROR("CryptAcquireContext failed: " + std::to_string(GetLastError()));
        return "";
    }

    if (!CryptGenRandom(hProv, (DWORD)length, buffer.data())) {
        LOG_ERROR("CryptGenRandom failed: " + std::to_string(GetLastError()));
        CryptReleaseContext(hProv, 0);
        return "";
    }

    CryptReleaseContext(hProv, 0);

    return std::string(buffer.begin(), buffer.end());
}

std::vector<uint8_t> Encryption::XOREncrypt(const std::vector<uint8_t>& data, const std::string& key) {
    std::vector<uint8_t> encrypted(data.size());
    size_t keyIndex = 0;

    for (size_t i = 0; i < data.size(); i++) {
        encrypted[i] = data[i] ^ key[keyIndex];
        keyIndex = (keyIndex + 1) % key.length();
    }

    return encrypted;
}

std::vector<uint8_t> Encryption::XORDecrypt(const std::vector<uint8_t>& encryptedData, const std::string& key) {
    return XOREncrypt(encryptedData, key); // XOR is symmetric
}