// KeyLoggerClient/include/utils/DataUtils.h

#ifndef DATAUTILS_H
#define DATAUTILS_H

#include <vector>
#include <cstdint>
#include <string>

namespace utils {
namespace DataUtils {

// Padding utilities
void AddRandomPadding(std::vector<uint8_t>& data, size_t minPadding, size_t maxPadding);
void RemovePadding(std::vector<uint8_t>& data);

// Data manipulation
std::vector<uint8_t> ConvertStringToBytes(const std::string& str);
std::string ConvertBytesToString(const std::vector<uint8_t>& bytes);

// Compression utilities
std::vector<uint8_t> CompressData(const std::vector<uint8_t>& data);
std::vector<uint8_t> DecompressData(const std::vector<uint8_t>& compressedData);

// Checksum utilities
uint32_t CalculateCRC32(const std::vector<uint8_t>& data);
uint16_t CalculateChecksum(const std::vector<uint8_t>& data);

// Encoding utilities
std::string Base64Encode(const std::vector<uint8_t>& data);
std::vector<uint8_t> Base64Decode(const std::string& encoded);

// Buffer utilities
std::vector<uint8_t> MergeVectors(const std::vector<uint8_t>& vec1, const std::vector<uint8_t>& vec2);
std::vector<uint8_t> SliceVector(const std::vector<uint8_t>& data, size_t start, size_t length);

} // namespace DataUtils
} // namespace utils

#endif // DATAUTILS_H