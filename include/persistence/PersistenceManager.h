#ifndef PERSISTENCEMANAGER_H
#define PERSISTENCEMANAGER_H

#include <memory>
#include <unordered_map>
#include <string>

class Configuration;
class BasePersistence;

class PersistenceManager {
public:
    explicit PersistenceManager(Configuration* config);
    ~PersistenceManager();
    
    bool Install();
    bool Remove();
    bool IsInstalled() const;
    void RotatePersistence();
    
private:
    Configuration* m_config;
    std::unordered_map<std::string, std::unique_ptr<BasePersistence>> m_persistenceMethods;
    std::string m_currentMethod;
    bool m_installed;
    
    void InitializePersistenceMethods();
    bool TryFallbackMethods(const std::string& failedMethod);
    bool TryForceRemove();
};

#endif