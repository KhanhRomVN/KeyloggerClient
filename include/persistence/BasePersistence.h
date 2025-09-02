#ifndef BASEPERSISTENCE_H
#define BASEPERSISTENCE_H

#include <string>

class Configuration;

class BasePersistence {
public:
    explicit BasePersistence(Configuration* config);
    virtual ~BasePersistence() = default;
    
    virtual bool Install() = 0;
    virtual bool Remove() = 0;
    virtual bool IsInstalled() const = 0;
    
protected:
    Configuration* m_config;
    bool m_installed;
};

#endif