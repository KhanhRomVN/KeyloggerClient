// KeyLoggerClient/include/utils/NetworkUtils.h
#ifndef NETWORKUTILS_H
#define NETWORKUTILS_H  

#include <string>
#include <vector>

namespace utils {

class NetworkUtils {
public:
    static bool CheckInternetConnection();
    static bool IsOnLocalNetwork();
    static std::string GetLocalGateway();
    static std::string GetCurrentSSID();
    static std::vector<std::string> GetNetworkAdapters();
    
private:
    static bool IsPrivateIP(const std::string& ip);
};

} // namespace utils

#endif