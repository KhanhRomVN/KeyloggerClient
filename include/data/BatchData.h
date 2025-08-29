#ifndef BATCHDATA_H
#define BATCHDATA_H

#include <string>
#include <vector>
#include <cstdint>

struct BatchHeader {
    std::string batchId;
    uint64_t startTime;
    uint64_t endTime;
    std::string clientId;
    uint32_t entryCount;
    uint32_t checksum;
};

struct BatchEntry {
    std::string timestamp;
    std::string dataType; // "key", "mouse", "system", "event"
    std::string data;
    uint16_t flags;
};

class BatchData {
public:
    BatchData();
    
    void SetHeader(const BatchHeader& header);
    void AddEntry(const BatchEntry& entry);
    
    [[nodiscard]] std::vector<uint8_t> Serialize() const;
    bool Deserialize(const std::vector<uint8_t>& data);
    
    [[nodiscard]] BatchHeader GetHeader() const;
    [[nodiscard]] std::vector<BatchEntry> GetEntries() const;
    
    [[nodiscard]] uint32_t CalculateChecksum() const;
    [[nodiscard]] bool ValidateChecksum() const;
    
private:
    BatchHeader m_header;
    std::vector<BatchEntry> m_entries;
    
    [[nodiscard]] std::string EncodeEntry(const BatchEntry& entry) const;
    [[nodiscard]] BatchEntry DecodeEntry(const std::string& encoded) const;
};

#endif