#include "data/Screenshot.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include <sstream>
#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <cstdio>
#include <mutex>

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// Global GDI+ token for initialization
ULONG_PTR g_gdiplusToken = 0;
std::once_flag g_gdiplusInitFlag;

#elif PLATFORM_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstring>
#include <png.h>
#include <jpeglib.h>
#endif

Screenshot::Screenshot()
    : m_width(0), m_height(0), m_bpp(0), m_timestamp(utils::TimeUtils::GetCurrentTimestamp()) {
    Initialize();
}

Screenshot::Screenshot(int width, int height, int bpp, const std::vector<uint8_t>& data)
    : m_width(width), m_height(height), m_bpp(bpp), m_imageData(data),
      m_timestamp(utils::TimeUtils::GetCurrentTimestamp()) {
    Initialize();
}

Screenshot::~Screenshot() {
    // Cleanup handled by static Cleanup() method
}

bool Screenshot::Capture() {
    return Capture(nullptr);
}

bool Screenshot::Capture(void* nativeHandle) {
#if PLATFORM_WINDOWS
    return CaptureWindows(static_cast<HWND>(nativeHandle));
#elif PLATFORM_LINUX
    return CaptureLinux();
#endif
}

void Screenshot::Initialize() {
#if PLATFORM_WINDOWS
    std::call_once(g_gdiplusInitFlag, []() {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
    });
#elif PLATFORM_LINUX
    // No special initialization needed for Linux
#endif
}

#if PLATFORM_WINDOWS
bool Screenshot::CaptureWindows(HWND hwnd) {
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
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
            LOG_ERROR("Invalid screen dimensions");
            return false;
        }

        // Create compatible DC and bitmap
        HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
        if (!hMemoryDC) {
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
            LOG_ERROR("Failed to create memory DC");
            return false;
        }

        HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
        if (!hBitmap) {
            DeleteDC(hMemoryDC);
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
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
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
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
            if (hwnd) ReleaseDC(hwnd, hScreenDC);
            else ReleaseDC(nullptr, hScreenDC);
            ReleaseDC(nullptr, hDC);
            LOG_ERROR("GetDIBits failed: " + std::to_string(error));
            return false;
        }

        // Cleanup
        SelectObject(hMemoryDC, hOldObject);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        if (hwnd) ReleaseDC(hwnd, hScreenDC);
        else ReleaseDC(nullptr, hScreenDC);
        ReleaseDC(nullptr, hDC);

        m_timestamp = utils::TimeUtils::GetCurrentTimestamp();

        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "Screenshot captured successfully: %dx%d, %zu bytes", 
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

#elif PLATFORM_LINUX
bool Screenshot::CaptureLinux() {
    try {
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            LOG_ERROR("Failed to open X display");
            return false;
        }

        Window root = DefaultRootWindow(display);
        XWindowAttributes attributes;
        XGetWindowAttributes(display, root, &attributes);

        m_width = attributes.width;
        m_height = attributes.height;

        XImage* image = XGetImage(display, root, 0, 0, m_width, m_height, AllPlanes, ZPixmap);
        if (!image) {
            XCloseDisplay(display);
            LOG_ERROR("Failed to get X image");
            return false;
        }

        // Convert XImage to RGB data
        m_bpp = 24; // Always 24-bit for compatibility
        size_t dataSize = m_width * m_height * 3;
        m_imageData.resize(dataSize);

        for (int y = 0; y < m_height; y++) {
            for (int x = 0; x < m_width; x++) {
                unsigned long pixel = XGetPixel(image, x, y);
                size_t index = (y * m_width + x) * 3;
                
                m_imageData[index] = (pixel >> 16) & 0xFF;     // Red
                m_imageData[index + 1] = (pixel >> 8) & 0xFF;  // Green
                m_imageData[index + 2] = pixel & 0xFF;         // Blue
            }
        }

        XDestroyImage(image);
        XCloseDisplay(display);

        m_timestamp = utils::TimeUtils::GetCurrentTimestamp();

        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "Screenshot captured successfully: %dx%d, %zu bytes", 
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
#endif

bool Screenshot::SaveToFile(const std::string& path) const {
    if (m_imageData.empty()) {
        LOG_ERROR("No image data to save");
        return false;
    }

#if PLATFORM_WINDOWS
    return SaveToFileWindows(path);
#elif PLATFORM_LINUX
    return SaveToFileLinux(path);
#endif
}

#if PLATFORM_WINDOWS
bool Screenshot::SaveToFileWindows(const std::string& path) const {
    try {
        // Convert string to wstring for Windows API
        std::wstring wpath = utils::StringUtils::Utf8ToWide(path);

        // Create bitmap from raw data
        Gdiplus::Bitmap bitmap(m_width, m_height, m_width * 3, 
                              PixelFormat24bppRGB, const_cast<uint8_t*>(m_imageData.data()));

        if (bitmap.GetLastStatus() != Gdiplus::Ok) {
            LOG_ERROR("Failed to create GDI+ bitmap from raw data");
            return false;
        }

        // Get PNG encoder CLSID
        CLSID clsidPng;
        if (GetEncoderClsid("image/png", &clsidPng) == -1) {
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
        Gdiplus::Status status = bitmap.Save(wpath.c_str(), &clsidPng, &encoderParams);
        
        if (status == Gdiplus::Ok) {
            char logMsg[512];
            snprintf(logMsg, sizeof(logMsg), "Screenshot saved to: %s", path.c_str());
            LOG_INFO(logMsg);
            return true;
        } else {
            char logMsg[512];
            snprintf(logMsg, sizeof(logMsg), "Failed to save screenshot to: %s", path.c_str());
            LOG_ERROR(logMsg);
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

#elif PLATFORM_LINUX
bool Screenshot::SaveToFileLinux(const std::string& path) const {
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp) {
        LOG_ERROR("Failed to open file for writing: " + path);
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        LOG_ERROR("Failed to create PNG write structure");
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        LOG_ERROR("Failed to create PNG info structure");
        return false;
    }

    if (_setjmp ((*png_set_longjmp_fn((png), longjmp, (sizeof (jmp_buf)))))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        LOG_ERROR("PNG error during write");
        return false;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, m_width, m_height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    // Write image data
    std::vector<png_bytep> row_pointers(m_height);
    for (int y = 0; y < m_height; y++) {
        row_pointers[y] = const_cast<png_byte*>(&m_imageData[y * m_width * 3]);
    }

    png_write_image(png, row_pointers.data());
    png_write_end(png, nullptr);

    png_destroy_write_struct(&png, &info);
    fclose(fp);

    char logMsg[512];
    snprintf(logMsg, sizeof(logMsg), "Screenshot saved to: %s", path.c_str());
    LOG_INFO(logMsg);

    return true;
}
#endif

std::vector<uint8_t> Screenshot::Compress(int quality) const {
    if (m_imageData.empty()) {
        LOG_ERROR("No image data to compress");
        return {};
    }

#if PLATFORM_WINDOWS
    return CompressWindows(quality);
#elif PLATFORM_LINUX
    return CompressLinux(quality);
#endif
}

#if PLATFORM_WINDOWS
std::vector<uint8_t> Screenshot::CompressWindows(int quality) const {
    try {
        // Create source bitmap
        Gdiplus::Bitmap sourceBitmap(m_width, m_height, m_width * 3, 
                                   PixelFormat24bppRGB, const_cast<uint8_t*>(m_imageData.data()));

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
        if (GetEncoderClsid("image/jpeg", &clsidJpeg) == -1) {
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
        snprintf(logMsg, sizeof(logMsg), "Screenshot compressed: %zu -> %zu bytes (%.1f%%)",
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

#elif PLATFORM_LINUX
std::vector<uint8_t> Screenshot::CompressLinux(int quality) const {
    try {
        struct jpeg_compress_struct cinfo{};
        struct jpeg_error_mgr jerr{};
        
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        
        unsigned char* buffer = nullptr;
        unsigned long bufferSize = 0;
        
        jpeg_mem_dest(&cinfo, &buffer, &bufferSize);
        
        cinfo.image_width = m_width;
        cinfo.image_height = m_height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);
        jpeg_start_compress(&cinfo, TRUE);
        
        JSAMPROW row_pointer[1];
        while (cinfo.next_scanline < cinfo.image_height) {
            row_pointer[0] = const_cast<JSAMPROW>(&m_imageData[cinfo.next_scanline * m_width * 3]);
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
        
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        
        std::vector<uint8_t> compressedData(buffer, buffer + bufferSize);
        free(buffer);
        
        // Log compression results
        double compressionRatio = (1.0 - static_cast<double>(compressedData.size()) / m_imageData.size()) * 100.0;
        char logMsg[256];
        snprintf(logMsg, sizeof(logMsg), "Screenshot compressed: %zu -> %zu bytes (%.1f%%)",
                m_imageData.size(), compressedData.size(), compressionRatio);
        LOG_INFO(logMsg);
        
        return compressedData;
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Exception in image compression: ") + e.what());
        return {};
    }
    catch (...) {
        LOG_ERROR("Unknown exception in image compression");
        return {};
    }
}
#endif

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

int Screenshot::GetEncoderClsid(const char* format, void* pClsid) {
#if PLATFORM_WINDOWS
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

    // Convert format to wstring
    std::wstring wformat = utils::StringUtils::Utf8ToWide(format);

    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pImageCodecInfo[i].MimeType, wformat.c_str()) == 0) {
            *static_cast<CLSID*>(pClsid) = pImageCodecInfo[i].Clsid;
            return i;
        }
    }

    return -1;
#elif PLATFORM_LINUX
    // Not needed for Linux implementation
    return -1;
#endif
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

#if PLATFORM_WINDOWS
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
#elif PLATFORM_LINUX
    // Linux typically only has one main screen
    Screenshot screenshot;
    if (screenshot.Capture()) {
        screenshots.push_back(std::move(screenshot));
    }
#endif

    return screenshots;
}

void Screenshot::Cleanup() {
#if PLATFORM_WINDOWS
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
#endif
}