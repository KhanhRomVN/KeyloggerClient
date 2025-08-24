#ifndef SERVICEPERSISTENCE_H
#define SERVICEPERSISTENCE_H

#include "persistence/BasePersistence.h"
#include <windows.h>

class Configuration;

class ServicePersistence : public BasePersistence {
public:
    explicit ServicePersistence(Configuration* config);
    
    bool Install() override;
    bool Remove() override;
    bool IsInstalled() const override;
    
    bool StartService();
    bool StopService();
};

#endif