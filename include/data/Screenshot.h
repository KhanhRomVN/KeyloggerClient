#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <vector>
#include <string>
#include <mutex>
#include "core/Platform.h"

#if PLATFORM_WINDOWS
#include <windows.h>
#endif

class Screenshot {
public:
    Screenshot();
    Screenshot(int width, int height, int bpp, const std::vector<uint8_t>& data);
    ~Screenshot();
    
    bool Capture();
    bool Capture(void* nativeHandle);
    [[nodiscard]] bool SaveToFile(const std::string& path) const;
    [[nodiscard]] std::vector<uint8_t> Compress(int quality = 85) const;
    
    [[nodiscard]] std::vector<uint8_t> GetImageData() const;
    [[nodiscard]] int GetWidth() const;
    [[nodiscard]] int GetHeight() const;
    [[nodiscard]] int GetBPP() const;
    [[nodiscard]] std::string GetTimestamp() const;
    [[nodiscard]] size_t GetSize() const;
    [[nodiscard]] bool IsValid() const;
    
    static std::vector<uint8_t> CaptureToMemory(int quality = 85);
    static bool CaptureToFile(const std::string& path, int quality = 85);
    static std::vector<Screenshot> CaptureMultipleDisplays();
    static void Cleanup();
    
private:
    int m_width;
    int m_height;
    int m_bpp;
    std::vector<uint8_t> m_imageData;
    std::string m_timestamp;
    
    void Initialize();
    static int GetEncoderClsid(const char* format, void* pClsid);
    
#if PLATFORM_WINDOWS
    bool CaptureWindows(HWND hwnd);
    bool SaveToFileWindows(const std::string& path) const;
    std::vector<uint8_t> CompressWindows(int quality) const;
#elif PLATFORM_LINUX
    bool CaptureLinux();
    [[nodiscard]] bool SaveToFileLinux(const std::string& path) const;
    [[nodiscard]] std::vector<uint8_t> CompressLinux(int quality) const;
#endif
};

#endif