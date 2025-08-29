#ifndef SERVICEPERSISTENCE_H
#define SERVICEPERSISTENCE_H

#include "persistence/BasePersistence.h"
#include "core/Platform.h"

class Configuration;

class ServicePersistence : public BasePersistence {
public:
    explicit ServicePersistence(Configuration* config);
    
    bool Install() override;
    bool Remove() override;
    bool IsInstalled() const override;
    
    static bool StartService();
    static bool StopService();
};

#endif