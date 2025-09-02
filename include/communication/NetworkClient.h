#ifndef INCLUDE_COMMUNICATION_NETWORKCLIENT_H
#define INCLUDE_COMMUNICATION_NETWORKCLIENT_H

/**
 * @file NetworkClient.h
 * @brief Network communication client
 * @date 2025-09-01
 */

#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Communication {

/**
 * @class NetworkClient
 * @brief Network communication client
 */
class NetworkClient {
public:
    NetworkClient();
    virtual ~NetworkClient();
    
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
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;
};

} // namespace Communication

#endif // INCLUDE_COMMUNICATION_NETWORKCLIENT_H
