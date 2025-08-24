#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <vector>
#include <string>
#include <windows.h>

class Screenshot {
public:
    Screenshot();
    Screenshot(int width, int height, int bpp, const std::vector<uint8_t>& data);
    ~Screenshot();
    
    bool Capture();
    bool Capture(HWND hwnd);
    bool SaveToFile(const std::wstring& path) const;
    std::vector<uint8_t> Compress(int quality = 85) const;
    
    std::vector<uint8_t> GetImageData() const;
    int GetWidth() const;
    int GetHeight() const;
    int GetBPP() const;
    std::string GetTimestamp() const;
    size_t GetSize() const;
    bool IsValid() const;
    
    static std::vector<uint8_t> CaptureToMemory(int quality = 85);
    static bool CaptureToFile(const std::wstring& path, int quality = 85);
    static std::vector<Screenshot> CaptureMultipleDisplays();
    static void CleanupGDIplus();
    
private:
    int m_width;
    int m_height;
    int m_bpp;
    std::vector<uint8_t> m_imageData;
    std::string m_timestamp;
    
    void InitializeGDIplus();
    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
};

#endif