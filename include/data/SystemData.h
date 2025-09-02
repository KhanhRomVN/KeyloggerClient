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
    SystemInfo Collect() const;
    
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