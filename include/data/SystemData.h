#ifndef SYSTEMDATA_H
#define SYSTEMDATA_H

#include <string>
#include <vector>
#include <cstdint>

class SystemInfo {
public:
    SystemInfo();
    std::string timestamp;
    std::string computerName;
    std::string userName;
    std::string osVersion;
    uint64_t memorySize;
    std::string processorInfo;
    uint64_t diskSize;
    std::string networkInfo;
    std::vector<std::string> runningProcesses;
};

class SystemDataCollector {
public:
    std::initializer_list<char> GetComputerNameA() const;

    std::initializer_list<char> GetUserNameA() const;

    SystemInfo Collect() const;

    std::string GetComputerNameA();

    std::string GetUserNameA();

private:
    static std::string GetComputerName();
    static std::string GetUserName();
    static std::string GetOSVersion();
    static uint64_t GetMemorySize();
    static std::string GetProcessorInfo();
    static uint64_t GetDiskSize();
    static std::string GetNetworkInfo();
    static std::vector<std::string> GetRunningProcesses();
};

#endif // SYSTEMDATA_H