#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>

class Configuration;
class DataManager;
class CommsManager;
class PersistenceManager;
class KeyHook;
class MouseHook;

class Application {
public:
    Application();
    ~Application();
    
    bool Initialize();
    void Run();
    void Shutdown();
    
private:
    bool m_running;
    std::unique_ptr<Configuration> m_config;
    std::unique_ptr<DataManager> m_dataManager;
    std::unique_ptr<CommsManager> m_commsManager;
    std::unique_ptr<PersistenceManager> m_persistenceManager;
    std::unique_ptr<KeyHook> m_keyHook;
    std::unique_ptr<MouseHook> m_mouseHook;
};

#endif