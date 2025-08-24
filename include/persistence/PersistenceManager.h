// File: PersistenceManager.h
#pragma once

#include "RegistryPersistence.h"
#include "SchedulePersistence.h"
#include "ServicePersistence.h"
#include "core/Logger.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>

namespace persistence {

class PersistenceManager {
public:
    enum class PersistenceMethod {
        REGISTRY,
        SCHEDULED_TASK,
        WINDOWS_SERVICE,
        STARTUP_FOLDER
    };

    PersistenceManager(std::shared_ptr<core::Logger> logger);
    ~PersistenceManager();

    // Delete copy constructor and assignment operator
    PersistenceManager(const PersistenceManager&) = delete;
    PersistenceManager& operator=(const PersistenceManager&) = delete;

    /**
     * @brief Installs persistence using specified methods
     * @param methods Vector of persistence methods to install
     * @param executablePath Path to the executable to persist
     * @return true if at least one method was successful
     */
    bool installPersistence(const std::vector<PersistenceMethod>& methods, 
                           const std::string& executablePath);

    /**
     * @brief Removes all installed persistence mechanisms
     * @return true if all removals were successful
     */
    bool removePersistence();

    /**
     * @brief Checks if any persistence method is currently active
     * @return true if at least one method is active
     */
    bool isPersistent() const;

    /**
     * @brief Attempts privilege escalation for persistence installation
     * @return true if escalation was successful
     */
    bool attemptPrivilegeEscalation();

    /**
     * @brief Emergency removal of all persistence mechanisms
     */
    void emergencyRemove();

    /**
     * @brief Gets status of all persistence methods
     * @return Vector of status messages
     */
    std::vector<std::string> getStatus() const;

private:
    /**
     * @brief Validates executable path for persistence
     */
    bool validateExecutablePath(const std::string& path) const;

    std::unique_ptr<RegistryPersistence> registryPersistence_;
    std::unique_ptr<SchedulePersistence> schedulePersistence_;
    std::unique_ptr<ServicePersistence> servicePersistence_;
    
    std::shared_ptr<core::Logger> logger_;
    mutable std::mutex mutex_;
    
    std::string currentExecutablePath_;
    std::vector<PersistenceMethod> activeMethods_;
};

} // namespace persistence