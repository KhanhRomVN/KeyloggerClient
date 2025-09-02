#ifndef REGISTRYPERSISTENCE_H
#define REGISTRYPERSISTENCE_H

#include "persistence/BasePersistence.h"
#include <windows.h>

class Configuration;

class RegistryPersistence : public BasePersistence {
public:
    explicit RegistryPersistence(Configuration* config);
    
    bool Install() override;
    bool Remove() override;
    bool IsInstalled() const override;
    
private:
    HKEY m_installedHive;
    std::string m_installedKey;
    
    bool InstallInRegistry(HKEY hive, const char* key);
    bool CheckRegistryKey(HKEY hive, const char* key) const;
};

#endif