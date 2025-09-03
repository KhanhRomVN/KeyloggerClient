#ifndef SCHEDULEPERSISTENCE_H
#define SCHEDULEPERSISTENCE_H

#include <taskschd.h>

#include "persistence/BasePersistence.h"

class Configuration;

class SchedulePersistence : public BasePersistence {
public:
    explicit SchedulePersistence(Configuration* config);
    ~SchedulePersistence();
    
    bool Install() override;

    bool CreateScheduledTask(ITaskService *pService);

    bool Remove() override;
    bool IsInstalled() const override;
    
private:
    bool CreateScheduledTask(void* pService);
};

#endif