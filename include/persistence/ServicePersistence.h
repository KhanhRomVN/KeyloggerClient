// File: ServicePersistence.h
#pragma once

#include "core/Logger.h"
#include <string>
#include <windows.h>

namespace persistence {

class ServicePersistence {
public:
    ServicePersistence(std::shared_ptr<core::Logger> logger);
    ~ServicePersistence();

    // Delete copy constructor and assignment operator
    ServicePersistence(const ServicePersistence&) = delete;
    ServicePersistence& operator=(const ServicePersistence&) = delete;

    /**
     * @brief Installs Windows service for persistence
     * @param serviceName Name of the service
     * @param displayName Display name of the service
     * @param executablePath Path to the executable
     * @param startType Service start type (AUTO, DEMAND, etc.)
     * @return true if installation successful
     */
    bool install(const std::string& serviceName, const std::string& displayName,
                const std::string& executablePath, DWORD startType);

    /**
     * @brief Removes Windows service
     * @param serviceName Name of the service to remove
     * @return true if removal successful
     */
    bool remove(const std::string& serviceName);

    /**
     * @brief Checks if service exists
     * @param serviceName Name of the service to check
     * @return true if service exists
     */
    bool exists(const std::string& serviceName) const;

    /**
     * @brief Starts the service
     * @param serviceName Name of the service to start
     * @return true if start successful
     */
    bool startService(const std::string& serviceName);

    /**
     * @brief Configures service failure actions
     */
    bool configureFailureActions(const std::string& serviceName);

    /**
     * @brief Emergency removal of all known services
     */
    void emergencyRemove();

private:
    /**
     * @brief Opens service control manager
     */
    bool openSCM(DWORD desiredAccess);

    /**
     * @brief Validates service parameters
     */
    bool validateServiceParameters(const std::string& serviceName, 
                                 const std::string& executablePath) const;

    SC_HANDLE scmHandle_;
    std::shared_ptr<core::Logger> logger_;
};

} // namespace persistence