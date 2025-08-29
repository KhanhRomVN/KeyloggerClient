#include "security/Encryption.h"
#include "core/Logger.h"
#include "utils/StringUtils.h"
#include "core/Platform.h"

#include <vector>
#include <cstdint>
#include <string>
#include <random>

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")
#elif PLATFORM_LINUX
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <sys/random.h>
#endif

namespace security {

std::vector<uint8_t> Encryption::EncryptAES(const std::vector<uint8_t>& data, const std::string& key) {
#if PLATFORM_WINDOWS
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
#elif PLATFORM_LINUX
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        LOG_ERROR("Failed to create EVP cipher context");
        return {};
    }

    // Generate random IV
    std::vector<uint8_t> iv(16);
    if (RAND_bytes(iv.data(), iv.size()) != 1) {
        LOG_ERROR("Failed to generate random IV");
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    // Derive key using SHA256
    std::vector<uint8_t> derivedKey(32); // 256 bits
    SHA256(reinterpret_cast<const unsigned char*>(key.c_str()), key.length(), derivedKey.data());

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derivedKey.data(), iv.data()) != 1) {
        LOG_ERROR("Failed to initialize encryption");
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    std::vector<uint8_t> encryptedData(data.size() + AES_BLOCK_SIZE);
    int len;

    if (EVP_EncryptUpdate(ctx, encryptedData.data(), &len, data.data(), data.size()) != 1) {
        LOG_ERROR("Encryption failed");
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    int ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, encryptedData.data() + len, &len) != 1) {
        LOG_ERROR("Encryption finalization failed");
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    ciphertext_len += len;

    // Prepend IV to encrypted data
    encryptedData.insert(encryptedData.begin(), iv.begin(), iv.end());
    encryptedData.resize(iv.size() + ciphertext_len);

    EVP_CIPHER_CTX_free(ctx);
    return encryptedData;
#endif
}

std::vector<uint8_t> Encryption::DecryptAES(const std::vector<uint8_t>& encryptedData, const std::string& key) {
    if (encryptedData.size() <= 16) {
        LOG_ERROR("Invalid encrypted data size");
        return {};
    }

#if PLATFORM_WINDOWS
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
#elif PLATFORM_LINUX
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        LOG_ERROR("Failed to create EVP cipher context");
        return {};
    }

    // Extract IV from beginning of encrypted data
    std::vector<uint8_t> iv(encryptedData.begin(), encryptedData.begin() + 16);
    std::vector<uint8_t> ciphertext(encryptedData.begin() + 16, encryptedData.end());

    // Derive key using SHA256
    std::vector<uint8_t> derivedKey(32); // 256 bits
    SHA256(reinterpret_cast<const unsigned char*>(key.c_str()), key.length(), derivedKey.data());

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derivedKey.data(), iv.data()) != 1) {
        LOG_ERROR("Failed to initialize decryption");
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    std::vector<uint8_t> decryptedData(ciphertext.size());
    int len;

    if (EVP_DecryptUpdate(ctx, decryptedData.data(), &len, ciphertext.data(), ciphertext.size()) != 1) {
        LOG_ERROR("Decryption failed");
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    int plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, decryptedData.data() + len, &len) != 1) {
        LOG_ERROR("Decryption finalization failed");
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    plaintext_len += len;

    decryptedData.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);
    return decryptedData;
#endif
}

std::string Encryption::GenerateSHA256(const std::string& input) {
#if PLATFORM_WINDOWS
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
#elif PLATFORM_LINUX
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    
    if (!SHA256_Init(&sha256)) {
        LOG_ERROR("SHA256 initialization failed");
        return "";
    }
    
    if (!SHA256_Update(&sha256, input.c_str(), input.length())) {
        LOG_ERROR("SHA256 update failed");
        return "";
    }
    
    if (!SHA256_Final(hash, &sha256)) {
        LOG_ERROR("SHA256 finalization failed");
        return "";
    }
    
    std::string hashStr;
    char buf[3];
    for (const unsigned char i : hash) {
        snprintf(buf, sizeof(buf), "%02x", i);
        hashStr += buf;
    }
    
    return hashStr;
#endif
}

std::string Encryption::GenerateRandomKey(size_t length) {
#if PLATFORM_WINDOWS
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
#elif PLATFORM_LINUX
    std::vector<uint8_t> buffer(length);
    
    if (getrandom(buffer.data(), length, 0) != static_cast<ssize_t>(length)) {
        LOG_ERROR("Failed to generate random bytes");
        return "";
    }
    
    return std::basic_string<char>(buffer.begin(), buffer.end());
#endif
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

} // namespace security