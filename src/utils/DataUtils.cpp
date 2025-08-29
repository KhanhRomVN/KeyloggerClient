// KeyLoggerClient/src/utils/DataUtils.cpp

#include "utils/DataUtils.h"
#include <random>
#include <algorithm>
#include <stdexcept>
#include <zlib.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

namespace utils::DataUtils {

void AddRandomPadding(std::vector<uint8_t>& data, size_t minPadding, size_t maxPadding) {
    if (minPadding > maxPadding) {
        std::swap(minPadding, maxPadding);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(minPadding, maxPadding);
    std::uniform_int_distribution<uint8_t> byteDist(0, 255);
    
    size_t paddingSize = dist(gen);
    std::vector<uint8_t> padding;
    padding.reserve(paddingSize);
    
    for (size_t i = 0; i < paddingSize; ++i) {
        padding.push_back(byteDist(gen));
    }
    
    // Insert padding at random position
    std::uniform_int_distribution<size_t> posDist(0, data.size());
    size_t insertPos = posDist(gen);
    
    data.insert(data.begin() + insertPos, padding.begin(), padding.end());
}

void RemovePadding(std::vector<uint8_t>& data) {
    // This is a placeholder - in real implementation, you'd need a way
    // to identify and remove the padding that was added
    // For now, we'll assume the padding is at the end
    if (!data.empty()) {
        size_t originalSize = data.size();
        // Simple heuristic: look for significant changes in byte patterns
        // This is a simplified approach
        data.resize(originalSize); // No actual removal in this basic implementation
    }
}

std::vector<uint8_t> ConvertStringToBytes(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::string ConvertBytesToString(const std::vector<uint8_t>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

std::vector<uint8_t> CompressData(const std::vector<uint8_t>& data) {
    if (data.empty()) return {};
    
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    
    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib compression");
    }
    
    stream.next_in = const_cast<Bytef*>(data.data());
    stream.avail_in = data.size();
    
    std::vector<uint8_t> compressed;
    compressed.resize(data.size() * 1.1 + 12); // Estimate size
    
    do {
        stream.next_out = compressed.data() + stream.total_out;
        stream.avail_out = compressed.size() - stream.total_out;
        
        int ret = deflate(&stream, Z_FINISH);
        
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&stream);
            throw std::runtime_error("Compression stream error");
        }
        
        if (stream.avail_out == 0) {
            compressed.resize(compressed.size() * 2);
        }
    } while (stream.avail_out == 0);
    
    deflateEnd(&stream);
    compressed.resize(stream.total_out);
    
    return compressed;
}

std::vector<uint8_t> DecompressData(const std::vector<uint8_t>& compressedData) {
    if (compressedData.empty()) return {};
    
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;
    
    if (inflateInit(&stream) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib decompression");
    }
    
    stream.next_in = const_cast<Bytef*>(compressedData.data());
    stream.avail_in = compressedData.size();
    
    std::vector<uint8_t> decompressed;
    decompressed.resize(compressedData.size() * 2);
    
    int ret;
    do {
        stream.next_out = decompressed.data() + stream.total_out;
        stream.avail_out = decompressed.size() - stream.total_out;
        
        ret = inflate(&stream, Z_NO_FLUSH);
        
        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&stream);
                throw std::runtime_error("Decompression error");
            default: ;
        }
        
        if (stream.avail_out == 0) {
            decompressed.resize(decompressed.size() * 2);
        }
    } while (ret != Z_STREAM_END);
    
    inflateEnd(&stream);
    decompressed.resize(stream.total_out);
    
    return decompressed;
}

uint32_t CalculateCRC32(const std::vector<uint8_t>& data) {
    uint32_t crc = 0xFFFFFFFF;

    for (uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            constexpr uint32_t polynomial = 0xEDB88320;
            crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
        }
    }
    
    return ~crc;
}

uint16_t CalculateChecksum(const std::vector<uint8_t>& data) {
    uint32_t sum = 0;
    size_t i = 0;
    
    // Sum 16-bit words
    while (i < data.size()) {
        uint16_t word = 0;
        if (i < data.size()) word |= data[i] << 8;
        if (i + 1 < data.size()) word |= data[i + 1];
        sum += word;
        i += 2;
    }
    
    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~static_cast<uint16_t>(sum);
}

std::string Base64Encode(const std::vector<uint8_t>& data) {
    BIO* bio = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    bio = BIO_push(bio, bmem);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data.data(), data.size());
    BIO_flush(bio);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);
    
    std::string result(bptr->data, bptr->length);
    
    BIO_free_all(bio);
    
    return result;
}

std::vector<uint8_t> Base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.data(), encoded.size());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    
    std::vector<uint8_t> decoded(encoded.size());
    int length = BIO_read(bio, decoded.data(), encoded.size());
    
    if (length < 0) {
        BIO_free_all(bio);
        throw std::runtime_error("Base64 decoding failed");
    }
    
    decoded.resize(length);
    BIO_free_all(bio);
    
    return decoded;
}

std::vector<uint8_t> MergeVectors(const std::vector<uint8_t>& vec1, const std::vector<uint8_t>& vec2) {
    std::vector<uint8_t> result;
    result.reserve(vec1.size() + vec2.size());
    result.insert(result.end(), vec1.begin(), vec1.end());
    result.insert(result.end(), vec2.begin(), vec2.end());
    return result;
}

std::vector<uint8_t> SliceVector(const std::vector<uint8_t>& data, size_t start, size_t length) {
    if (start >= data.size()) {
        return {};
    }
    
    size_t end = std::min(start + length, data.size());
    return std::vector<uint8_t>(data.begin() + start, data.begin() + end);
}

} // namespace utils::DataUtils
