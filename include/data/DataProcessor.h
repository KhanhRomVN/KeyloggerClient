#ifndef INCLUDE_DATA_DATAPROCESSOR_H
#define INCLUDE_DATA_DATAPROCESSOR_H

/**
 * @file DataProcessor.h
 * @brief Data processing and formatting
 * @date 2025-09-01
 */

#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Data {

/**
 * @class DataProcessor
 * @brief Data processing and formatting
 */
class DataProcessor {
public:
    DataProcessor();
    virtual ~DataProcessor();
    
    // Core interface
    virtual bool initialize();
    virtual void cleanup();
    virtual bool isInitialized() const { return initialized_; }
    
    // Status and information
    virtual std::string getStatus() const;
    virtual std::string getVersion() const { return "1.0.0"; }
    
protected:
    bool initialized_;
    std::string last_error_;
    
    // Error handling
    void setLastError(const std::string& error) { last_error_ = error; }
    
private:
    // Prevent copying
    DataProcessor(const DataProcessor&) = delete;
    DataProcessor& operator=(const DataProcessor&) = delete;
};

} // namespace Data

#endif // INCLUDE_DATA_DATAPROCESSOR_H
