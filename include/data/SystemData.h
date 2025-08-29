#ifndef SYSTEMDATA_H
#define SYSTEMDATA_H

#include <string>
#include <vector>
#include <cstdint>
#include "core/Platform.h"  

struct SystemInfo {
    std::string timestamp;
    std::string computerName;
    std::string userName;
    std::string osVersion;
    uint64_t memorySize;
    std::string processorInfo;
    uint64_t diskSize;
    std::string networkInfo;
    std::vector<std::string> runningProcesses;
    
    SystemInfo();
};

class SystemDataCollector {
public:
    SystemDataCollector();

    std::initializer_list<char> GetOSVersion() const;

    SystemInfo Collect() const;

    std::string GetComputerName();

    std::string GetUserName();

    static std::string GetOSVersion();

    static uint64_t GetMemorySize();

    static std::string GetProcessorInfo();

    static uint64_t GetDiskSize();

    static std::string GetNetworkInfo();

    static std::vector<std::string> GetRunningProcesses();

private:
    std::string GetComputerName() const;
    std::string GetUserName() const;
    std::string GetOSVersion() const;
    uint64_t GetMemorySize() const;
    std::string GetProcessorInfo() const;
    uint64_t GetDiskSize() const;
    std::string GetNetworkInfo() const;
    std::vector<std::string> GetRunningProcesses() const;
};

#endif