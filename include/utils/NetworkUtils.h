// KeyLoggerClient/include/utils/NetworkUtils.h
#ifndef NETWORKUTILS_H
#define NETWORKUTILS_H

#include <string>

class NetworkUtils {
public:
    static bool IsOnLocalNetwork();
    static std::string GetLocalGateway();
    static std::string GetCurrentSSID();
};

#endif