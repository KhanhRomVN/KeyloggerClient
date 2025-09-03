#include "data/Screenshot.h"
#include <windows.h>
#include <gdiplus.h>
#include <sstream>
#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <cstdio>
#include <mutex>
#include <objbase.h> // For IStream and related functions

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib") // For CreateStreamOnHGlobal

using namespace Gdiplus;

// Global GDI+ token for initialization
ULONG_PTR g_gdiplusToken = 0;
std::once_flag g_gdiplusInitFlag;

Screenshot::Screenshot()
    : m_width(0), m_height(0), m_bpp(0), m_timestamp("") {
    Initialize();
}

Screenshot::Screenshot(int width, int height, int bpp, const std::vector<uint8_t>& data)
    : m_width(width), m_height(height), m_bpp(bpp), m_imageData(data),
      m_timestamp("") {
    Initialize();
}

Screenshot::~Screenshot() {
    // Cleanup handled by static Cleanup() method
}

bool Screenshot::Capture() {
    return Capture(nullptr);
}

bool Screenshot::Capture(void* nativeHandle) {
    return CaptureWindows(static_cast<HWND>(nativeHandle));
}

void Screenshot::Initialize() {
    std::call_once(g_gdiplusInitFlag, []() {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
    });
}

bool Screenshot::CaptureWindows(HWND hwnd) {
    try {
        HDC hScreenDC = (hwnd) ? GetDC(hwnd) : GetDC(nullptr);
        if (!hScreenDC) {
            return false;
        }

        // Get screen dimensions
        int x, y, width, height;
        if (hwnd) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            x = rect.left;
            y = rect.top;
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        } else {
            x = GetSystemMetrics(SM_XVIRTUALSCREEN);
            y = GetSystemMetrics(SM_YVIRTUALSCREEN);
            width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        }

        if (width <= 0 || height <= 0) {
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
            return false;
        }

        // Create compatible DC and bitmap
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
        if (!hMemoryDC) {
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
            return false;
        }

        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
        if (!hBitmap) {
            DeleteDC(hMemoryDC);
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
            return false;
        }

        // Select bitmap into memory DC
        HGDIOBJ hOldObject = SelectObject(hMemoryDC, hBitmap);

        // Copy screen to bitmap
        BOOL result = BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY);
        if (!result) {
            SelectObject(hMemoryDC, hOldObject);
            DeleteObject(hBitmap);
            DeleteDC(hMemoryDC);
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
            return false;
        }

        // Get bitmap information
        BITMAP bmp;
        GetObject(hBitmap, sizeof(BITMAP), &bmp);
        m_bpp = bmp.bmBitsPixel;
        m_width = bmp.bmWidth;
        m_height = bmp.bmHeight;

        // Get bitmap bits
        BITMAPINFOHEADER bi = {0};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = m_width;
        bi.biHeight = -m_height; // Negative for top-down DIB
        bi.biPlanes = 1;
        bi.biBitCount = 24; // Force 24-bit for compatibility
        bi.biCompression = BI_RGB;

        // Calculate stride and buffer size
        DWORD dwStride = ((m_width * 24 + 31) / 32) * 4;
        DWORD dwBmpSize = dwStride * m_height;
        m_imageData.resize(dwBmpSize);

        // Get DIB bits
        HDC hDC = GetDC(nullptr);
        if (!GetDIBits(hDC, hBitmap, 0, m_height, m_imageData.data(), 
                      (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
            SelectObject(hMemoryDC, hOldObject);
            DeleteObject(hBitmap);
            DeleteDC(hMemoryDC);
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
            ReleaseDC(nullptr, hDC);
            return false;
        }

        // Cleanup
        SelectObject(hMemoryDC, hOldObject);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        if (hwnd) ReleaseDC(hwnd, hScreenDC);
        else ReleaseDC(nullptr, hScreenDC);
        ReleaseDC(nullptr, hDC);

        return true;
    }
    catch (...) {
        return false;
    }
}

bool Screenshot::SaveToFile(const std::string& path) const {
    if (m_imageData.empty()) {
        return false;
    }

    return SaveToFileWindows(path);
}

bool Screenshot::SaveToFileWindows(const std::string& path) const {
    try {
        // Convert string to wstring for Windows API
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), NULL, 0);
        std::wstring wpath(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), &wpath[0], size_needed);

        // Create bitmap from raw data
        Bitmap bitmap(m_width, m_height, m_width * 3, 
                              PixelFormat24bppRGB, const_cast<uint8_t*>(m_imageData.data()));

        if (bitmap.GetLastStatus() != Ok) {
            return false;
        }

        // Get PNG encoder CLSID
        CLSID clsidPng;
        if (GetEncoderClsid(L"image/png", &clsidPng) == -1) {
            return false;
        }

        // Save image
        Status status = bitmap.Save(wpath.c_str(), &clsidPng, nullptr);
        
        return status == Ok;
    }
    catch (...) {
        return false;
    }
}

std::vector<uint8_t> Screenshot::Compress(int quality) const {
    if (m_imageData.empty()) {
        return {};
    }

    return CompressWindows(quality);
}

std::vector<uint8_t> Screenshot::CompressWindows(int quality) const {
    try {
        // Create source bitmap
        Bitmap sourceBitmap(m_width, m_height, m_width * 3, 
                                   PixelFormat24bppRGB, const_cast<uint8_t*>(m_imageData.data()));

        if (sourceBitmap.GetLastStatus() != Ok) {
            return std::vector<uint8_t>();
        }

        // Create stream for compressed data
        IStream* pStream = nullptr;
        if (CreateStreamOnHGlobal(nullptr, TRUE, &pStream) != S_OK) {
            return std::vector<uint8_t>();
        }

        std::unique_ptr<IStream, void(*)(IStream*)> streamGuard(pStream, [](IStream* s) { s->Release(); });

        // Get JPEG encoder CLSID
        CLSID clsidJpeg;
        if (GetEncoderClsid(L"image/jpeg", &clsidJpeg) == -1) {
            return std::vector<uint8_t>();
        }

        // Set encoder parameters
        EncoderParameters encoderParams;
        encoderParams.Count = 1;
        encoderParams.Parameter[0].Guid = EncoderQuality;
        encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
        encoderParams.Parameter[0].NumberOfValues = 1;
        ULONG qualityValue = static_cast<ULONG>(quality);
        encoderParams.Parameter[0].Value = &qualityValue;

        // Save to stream
        Status status = sourceBitmap.Save(pStream, &clsidJpeg, &encoderParams);
        if (status != Ok) {
            return std::vector<uint8_t>();
        }

        // Get stream data
        HGLOBAL hGlobal = nullptr;
        if (GetHGlobalFromStream(pStream, &hGlobal) != S_OK) {
            return std::vector<uint8_t>();
        }

        SIZE_T streamSize = GlobalSize(hGlobal);
        void* pData = GlobalLock(hGlobal);
        if (!pData) {
            return std::vector<uint8_t>();
        }

        std::vector<uint8_t> compressedData(static_cast<uint8_t*>(pData), 
                                          static_cast<uint8_t*>(pData) + streamSize);

        GlobalUnlock(hGlobal);
        return compressedData;
    }
    catch (...) {
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> Screenshot::GetImageData() const {
    return m_imageData;
}

int Screenshot::GetWidth() const {
    return m_width;
}

int Screenshot::GetHeight() const {
    return m_height;
}

int Screenshot::GetBPP() const {
    return m_bpp;
}

std::string Screenshot::GetTimestamp() const {
    return m_timestamp;
}

size_t Screenshot::GetSize() const {
    return m_imageData.size();
}

bool Screenshot::IsValid() const {
    return !m_imageData.empty() && m_width > 0 && m_height > 0;
}

int Screenshot::GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;

    GetImageEncodersSize(&num, &size);
    if (size == 0) {
        return -1;
    }

    std::vector<uint8_t> buffer(size);
    ImageCodecInfo* pImageCodecInfo = reinterpret_cast<ImageCodecInfo*>(buffer.data());

    if (GetImageEncoders(num, size, pImageCodecInfo) != Ok) {
        return -1;
    }

    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[i].Clsid;
            return i;
        }
    }

    return -1;
}

std::vector<uint8_t> Screenshot::CaptureToMemory(int quality) {
    Screenshot screenshot;
    if (screenshot.Capture()) {
        return screenshot.Compress(quality);
    }
    return {};
}

bool Screenshot::CaptureToFile(const std::string& path, int quality) {
    Screenshot screenshot;
    if (screenshot.Capture()) {
        if (quality < 100) {
            auto compressedData = screenshot.Compress(quality);
            if (!compressedData.empty()) {
                FILE* file = fopen(path.c_str(), "wb");
                if (file) {
                    fwrite(compressedData.data(), 1, compressedData.size(), file);
                    fclose(file);
                    return true;
                }
            }
        } else {
            return screenshot.SaveToFile(path);
        }
    }
    return false;
}

std::vector<Screenshot> Screenshot::CaptureMultipleDisplays() {
    std::vector<Screenshot> screenshots;

    int displayCount = GetSystemMetrics(SM_CMONITORS);
    if (displayCount <= 0) {
        displayCount = 1; // Fallback to single display
    }

    for (int i = 0; i < displayCount; i++) {
        Screenshot screenshot;
        if (screenshot.Capture()) {
            screenshots.push_back(std::move(screenshot));
        }
    }

    return screenshots;
}

void Screenshot::Cleanup() {
    if (g_gdiplusToken) {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}