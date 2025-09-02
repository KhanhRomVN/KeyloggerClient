#ifndef INCLUDE_HOOKS_KEYBOARDHOOK_H
#define INCLUDE_HOOKS_KEYBOARDHOOK_H

/**
 * @file KeyboardHook.h
 * @brief Keyboard event capture
 * @date 2025-09-01
 */

#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Hooks {

/**
 * @class KeyboardHook
 * @brief Keyboard event capture
 */
class KeyboardHook {
public:
    KeyboardHook();
    virtual ~KeyboardHook();
    
    // Core interface
    virtual bool initialize();
    virtual void cleanup();
    virtual bool isInitialized() const { return initialized_; }
    
    // Status and information
    virtual std::string getStatus() const;
    virtual std::string getVersion() const { return "1.0.0"; }
    
protected:
    bool initialized_;
    std::string last_error_;
    
    // Error handling
    void setLastError(const std::string& error) { last_error_ = error; }
    
private:
    // Prevent copying
    KeyboardHook(const KeyboardHook&) = delete;
    KeyboardHook& operator=(const KeyboardHook&) = delete;
};

} // namespace Hooks

#endif // INCLUDE_HOOKS_KEYBOARDHOOK_H
