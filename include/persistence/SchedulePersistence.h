#ifndef SCHEDULEPERSISTENCE_H
#define SCHEDULEPERSISTENCE_H

#include "persistence/BasePersistence.h"

class Configuration;

class SchedulePersistence : public BasePersistence {
public:
    explicit SchedulePersistence(Configuration* config);
    ~SchedulePersistence();
    
    bool Install() override;
    bool Remove() override;
    bool IsInstalled() const override;
    
private:
    bool CreateScheduledTask(void* pService);
};

#endif