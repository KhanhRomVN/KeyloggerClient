#include "core/Application.h"
#include "core/Logger.h"
#include "obfuscate.h"
#include <vector>
#include <string>

// Obfuscated strings
constexpr auto OBF_MAIN = OBFUSCATE("Main");
constexpr auto OBF_STARTUP_SUCCESS = OBFUSCATE("Application started successfully");
constexpr auto OBF_STARTUP_FAILED = OBFUSCATE("Application failed to start");
constexpr auto OBF_SHUTDOWN = OBFUSCATE("Application shutdown complete");

int main(int argc, char* argv[]) {
    // Check if running as service
    bool runAsService = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--service") {
            runAsService = true;
            break;
        }   
    }

    // Anti-analysis checks
    if (security::AntiAnalysis::IsDebuggerPresent()) {
        return 0;
    }

    if (security::AntiAnalysis::IsRunningInVM() || security::AntiAnalysis::IsSandboxed()) {
        // Continue but use stealth mode
        utils::SystemUtils::EnableStealthMode();
    }

    try {
        Application app;
        if (app.Initialize()) {
            LOG_INFO(OBF_STARTUP_SUCCESS);
            app.Run();
        } else {
            LOG_ERROR(OBF_STARTUP_FAILED);
            return 1;
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Unhandled exception: ") + e.what());
        return 1;
    }
    catch (...) {
        LOG_ERROR("Unknown unhandled exception");
        return 1;
    }

    LOG_INFO(OBF_SHUTDOWN);
    return 0;
}

// Service entry point (Windows only)
#ifndef CROSS_COMPILE
void ServiceMain(int argc, char* argv[]) {
    // Service initialization code would go here
    main(argc, argv);
}

// Service control handler
void ServiceCtrlHandler(DWORD control) {
    switch (control) {
        case SERVICE_CONTROL_STOP:
            // Clean shutdown
            break;
        case SERVICE_CONTROL_INTERROGATE:
            break;
        default:
            break;
    }
}
#endif