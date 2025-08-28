#ifndef SCHEDULEPERSISTENCE_H
#define SCHEDULEPERSISTENCE_H

#include "persistence/BasePersistence.h"
#include "core/Platform.h"

class Configuration;

class SchedulePersistence : public BasePersistence {
public:
    explicit SchedulePersistence(Configuration* config);
    ~SchedulePersistence();
    
    bool Install() override;
    bool Remove() override;
    bool IsInstalled() const override;
    
private:
#if PLATFORM_WINDOWS
    bool CreateScheduledTask(void* pService);
#endif
};

#endif