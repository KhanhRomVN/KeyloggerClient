#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H
#include <string>
#include <vector>
#include <cstdint>

namespace utils {

struct SystemInfo {
    std::string computerName;
    std::string userName;
    std::string osVersion;  
    uint64_t memorySize;
    std::string processorInfo;
    uint64_t diskSize;
    std::string networkInfo;
    std::vector<std::string> runningProcesses;
    std::string timestamp;
};

class SystemUtils {
public:
    // Non-static methods
    std::string GetComputerNameA();
    
    // Static methods
    static std::string GetUserName();
    static std::string GetOSVersion();
    static uint64_t GetMemorySize();
    static std::string GetProcessorInfo();
    static std::string GetSystemFingerprint();
    
    static bool IsElevated();
    static bool EnableStealthMode();
    static bool IsExitTriggered();
    
    static SystemInfo CollectSystemInformation();
    static std::vector<std::string> GetRunningProcesses();
    static bool CheckInternetConnection();
    static std::string GetMACAddress();
    
    static void CriticalShutdown();
};

} // namespace utils

#endif