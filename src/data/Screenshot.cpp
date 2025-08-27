#include "data/Screenshot.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "utils/FileUtils.h"
#include "security/Obfuscation.h"
#include <Windows.h>
#include <gdiplus.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <cstdint>
#include <string>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// Obfuscated strings
constexpr auto OBF_SCREENSHOT = OBFUSCATE("Screenshot");
constexpr auto OBF_CAPTURE_SUCCESS = OBFUSCATE("Screenshot captured successfully: %dx%d, %zu bytes");
constexpr auto OBF_CAPTURE_FAILED = OBFUSCATE("Screenshot capture failed");
constexpr auto OBF_SAVE_SUCCESS = OBFUSCATE("Screenshot saved to: %s");
constexpr auto OBF_SAVE_FAILED = OBFUSCATE("Failed to save screenshot to: %s");
constexpr auto OBF_COMPRESS_SUCCESS = OBFUSCATE("Screenshot compressed: %zu -> %zu bytes (%.1f%%)");
constexpr auto OBF_COMPRESS_FAILED = OBFUSCATE("Screenshot compression failed");

// Global GDI+ token for initialization
ULONG_PTR g_gdiplusToken = 0;
std::once_flag g_gdiplusInitFlag;

Screenshot::Screenshot()
    : m_width(0), m_height(0), m_bpp(0), m_timestamp(utils::TimeUtils::GetCurrentTimestamp()) {
    InitializeGDIplus();
}

Screenshot::Screenshot(int width, int height, int bpp, const std::vector<uint8_t>& data)
    : m_width(width), m_height(height), m_bpp(bpp), m_imageData(data),
      m_timestamp(utils::TimeUtils::GetCurrentTimestamp()) {
    InitializeGDIplus();
}

Screenshot::~Screenshot() {
    // GDI+ is cleaned up automatically at process exit
}

void Screenshot::InitializeGDIplus() {
    std::call_once(g_gdiplusInitFlag, []() {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
    });
}

bool Screenshot::Capture() {
    return Capture(nullptr);
}

bool Screenshot::Capture(HWND hwnd) {
    try {
        HDC hScreenDC = (hwnd) ? GetDC(hwnd) : GetDC(nullptr);
        if (!hScreenDC) {
            LOG_ERROR("Failed to get device context");
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
            ReleaseDC(hwnd, hScreenDC);
            LOG_ERROR("Invalid screen dimensions");
            return false;
        }

        // Create compatible DC and bitmap
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
        if (!hMemoryDC) {
            ReleaseDC(hwnd, hScreenDC);
            LOG_ERROR("Failed to create memory DC");
            return false;
        }

        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
        if (!hBitmap) {
            DeleteDC(hMemoryDC);
            ReleaseDC(hwnd, hScreenDC);
            LOG_ERROR("Failed to create bitmap");
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
            ReleaseDC(hwnd, hScreenDC);
            LOG_ERROR("BitBlt failed: " + std::to_string(GetLastError()));
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
            DWORD error = GetLastError();
            SelectObject(hMemoryDC, hOldObject);
            DeleteObject(hBitmap);
            DeleteDC(hMemoryDC);
            ReleaseDC(hwnd, hScreenDC);
            ReleaseDC(nullptr, hDC);
            LOG_ERROR("GetDIBits failed: " + std::to_string(error));
            return false;
        }

        // Cleanup
        SelectObject(hMemoryDC, hOldObject);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(hwnd, hScreenDC);
        ReleaseDC(nullptr, hDC);

        m_timestamp = utils::TimeUtils::GetCurrentTimestamp();

        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), OBFUSCATED_CAPTURE_SUCCESS, 
                m_width, m_height, m_imageData.size());
        LOG_INFO(logMsg);

        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Exception in screenshot capture: ") + e.what());
        return false;
    }
    catch (...) {
        LOG_ERROR("Unknown exception in screenshot capture");
        return false;
    }
}

bool Screenshot::SaveToFile(const std::wstring& path) const {
    if (m_imageData.empty()) {
        LOG_ERROR("No image data to save");
        return false;
    }

    try {
        // Create bitmap from raw data
        Gdiplus::Bitmap bitmap(m_width, m_height, m_width * 3, 
                              PixelFormat24bppRGB, m_imageData.data());

        if (bitmap.GetLastStatus() != Gdiplus::Ok) {
            LOG_ERROR("Failed to create GDI+ bitmap from raw data");
            return false;
        }

        // Get PNG encoder CLSID
        CLSID clsidPng;
        if (GetEncoderClsid(L"image/png", &clsidPng) == -1) {
            LOG_ERROR("Failed to get PNG encoder CLSID");
            return false;
        }

        // Set encoder parameters for quality
        Gdiplus::EncoderParameters encoderParams;
        encoderParams.Count = 1;
        encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
        encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
        encoderParams.Parameter[0].NumberOfValues = 1;

        ULONG quality = 90; // 90% quality
        encoderParams.Parameter[0].Value = &quality;

        // Save image
        Gdiplus::Status status = bitmap.Save(path.c_str(), &clsidPng, &encoderParams);
        
        if (status == Gdiplus::Ok) {
            wchar_t logMsg[512];
            swprintf(logMsg, sizeof(logMsg)/sizeof(wchar_t), 
                    OBFUSCATED_SAVE_SUCCESS, path.c_str());
            LOG_INFO(utils::StringUtils::WideToUtf8(logMsg));
            return true;
        } else {
            wchar_t logMsg[512];
            swprintf(logMsg, sizeof(logMsg)/sizeof(wchar_t), 
                    OBFUSCATED_SAVE_FAILED, path.c_str());
            LOG_ERROR(utils::StringUtils::WideToUtf8(logMsg));
            return false;
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Exception saving screenshot: ") + e.what());
        return false;
    }
    catch (...) {
        LOG_ERROR("Unknown exception saving screenshot");
        return false;
    }
}

std::vector<uint8_t> Screenshot::Compress(int quality) const {
    if (m_imageData.empty()) {
        LOG_ERROR("No image data to compress");
        return std::vector<uint8_t>();
    }

    try {
        // Create source bitmap
        Gdiplus::Bitmap sourceBitmap(m_width, m_height, m_width * 3, 
                                   PixelFormat24bppRGB, m_imageData.data());

        if (sourceBitmap.GetLastStatus() != Gdiplus::Ok) {
            LOG_ERROR("Failed to create source bitmap for compression");
            return std::vector<uint8_t>();
        }

        // Create stream for compressed data
        IStream* pStream = nullptr;
        if (CreateStreamOnHGlobal(nullptr, TRUE, &pStream) != S_OK) {
            LOG_ERROR("Failed to create IStream");
            return std::vector<uint8_t>();
        }

        std::unique_ptr<IStream, void(*)(IStream*)> streamGuard(pStream, [](IStream* s) { s->Release(); });

        // Get JPEG encoder CLSID
        CLSID clsidJpeg;
        if (GetEncoderClsid(L"image/jpeg", &clsidJpeg) == -1) {
            LOG_ERROR("Failed to get JPEG encoder CLSID");
            return std::vector<uint8_t>();
        }

        // Set encoder parameters
        Gdiplus::EncoderParameters encoderParams;
        encoderParams.Count = 1;
        encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
        encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
        encoderParams.Parameter[0].NumberOfValues = 1;
        encoderParams.Parameter[0].Value = &quality;

        // Save to stream
        Gdiplus::Status status = sourceBitmap.Save(pStream, &clsidJpeg, &encoderParams);
        if (status != Gdiplus::Ok) {
            LOG_ERROR("Failed to compress image: " + std::to_string(status));
            return std::vector<uint8_t>();
        }

        // Get stream data
        HGLOBAL hGlobal = nullptr;
        if (GetHGlobalFromStream(pStream, &hGlobal) != S_OK) {
            LOG_ERROR("Failed to get HGLOBAL from stream");
            return std::vector<uint8_t>();
        }

        SIZE_T streamSize = GlobalSize(hGlobal);
        void* pData = GlobalLock(hGlobal);
        if (!pData) {
            LOG_ERROR("Failed to lock global memory");
            return std::vector<uint8_t>();
        }

        std::vector<uint8_t> compressedData(static_cast<uint8_t*>(pData), 
                                          static_cast<uint8_t*>(pData) + streamSize);

        GlobalUnlock(hGlobal);

        // Log compression results
        double compressionRatio = (1.0 - (double)compressedData.size() / m_imageData.size()) * 100.0;
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), OBFUSCATED_COMPRESS_SUCCESS,
                m_imageData.size(), compressedData.size(), compressionRatio);
        LOG_INFO(logMsg);

        return compressedData;
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Exception in image compression: ") + e.what());
        return std::vector<uint8_t>();
    }
    catch (...) {
        LOG_ERROR("Unknown exception in image compression");
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

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) {
        return -1;
    }

    std::vector<uint8_t> buffer(size);
    Gdiplus::ImageCodecInfo* pImageCodecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());

    if (Gdiplus::GetImageEncoders(num, size, pImageCodecInfo) != Gdiplus::Ok) {
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
    return std::vector<uint8_t>();
}

bool Screenshot::CaptureToFile(const std::wstring& path, int quality) {
    Screenshot screenshot;
    if (screenshot.Capture()) {
        if (quality < 100) {
            auto compressedData = screenshot.Compress(quality);
            if (!compressedData.empty()) {
                return utils::FileUtils::WriteBinaryFile(path, compressedData);
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
        // For multiple displays, we'd need to use EnumDisplayMonitors
        // This is a simplified version that captures each display individually
        Screenshot screenshot;
        if (screenshot.Capture()) {
            screenshots.push_back(std::move(screenshot));
        }
    }

    return screenshots;
}

void Screenshot::CleanupGDIplus() {
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}