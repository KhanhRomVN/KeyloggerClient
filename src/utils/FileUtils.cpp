// src/utils/FileUtils.cpp
#include "utils/FileUtils.h"
#include "core/Logger.h"
#include "security/Obfuscation.h"
#include <Windows.h>
#include <Shlobj.h> 
#include <fstream>
#include <random>
#include <chrono>
#include <vector>
#include <cstdint>
#include <string>

// Obfuscated strings
constexpr auto OBF_FILE_UTILS = OBFUSCATE("FileUtils");

std::wstring FileUtils::GetCurrentExecutablePath() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return std::wstring(buffer);
}

std::wstring FileUtils::GetCurrentModulePath() {
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)GetCurrentModulePath, &hModule);
    
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(hModule, buffer, MAX_PATH);
    return std::wstring(buffer);
}

std::wstring FileUtils::GetAppDataPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::wstring(path);
    }
    return GetTempPath();
}

std::wstring FileUtils::GetTempPath() {
    wchar_t buffer[MAX_PATH];
    ::GetTempPathW(MAX_PATH, buffer);
    return std::wstring(buffer);
}

std::wstring FileUtils::GetSystemPath() {
    wchar_t buffer[MAX_PATH];
    GetSystemDirectoryW(buffer, MAX_PATH);
    return std::wstring(buffer);
}

bool FileUtils::FileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}

bool FileUtils::DirectoryExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

bool FileUtils::CreateDirectories(const std::wstring& path) {
    if (DirectoryExists(path)) return true;

    size_t pos = 0;
    std::wstring dir = path;
    
    if (dir[dir.size() - 1] == '\\') {
        dir.pop_back();
    }

    while ((pos = dir.find_first_of(L"\\/", pos + 1)) != std::wstring::npos) {
        std::wstring subdir = dir.substr(0, pos);
        if (!DirectoryExists(subdir) && !CreateDirectoryW(subdir.c_str(), NULL)) {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS) {
                LOG_ERROR("Failed to create directory: " + StringUtils::WideToUtf8(subdir) + 
                         ", error: " + std::to_string(err));
                return false;
            }
        }
    }

    return CreateDirectoryW(dir.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool FileUtils::DeleteFile(const std::wstring& path) {
    if (!FileExists(path)) return true;
    
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs & FILE_ATTRIBUTE_READONLY) {
        SetFileAttributesW(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }
    
    return ::DeleteFileW(path.c_str()) != 0;
}

bool FileUtils::MoveFile(const std::wstring& from, const std::wstring& to) {
    return ::MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
}

bool FileUtils::CopyFile(const std::wstring& from, const std::wstring& to) {
    return ::CopyFileW(from.c_str(), to.c_str(), FALSE) != 0;
}

uint64_t FileUtils::GetFileSize(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) {
        return 0;
    }
    
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return size.QuadPart;
}

std::vector<uint8_t> FileUtils::ReadBinaryFile(const std::wstring& path) {
    std::vector<uint8_t> data;
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, 
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to open file for reading: " + StringUtils::WideToUtf8(path) +
                 ", error: " + std::to_string(GetLastError()));
        return data;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return data;
    }

    data.resize(fileSize.QuadPart);
    DWORD bytesRead;
    if (!ReadFile(hFile, data.data(), (DWORD)data.size(), &bytesRead, NULL)) {
        LOG_ERROR("Failed to read file: " + StringUtils::WideToUtf8(path) +
                 ", error: " + std::to_string(GetLastError()));
        data.clear();
    }

    CloseHandle(hFile);
    return data;
}

bool FileUtils::WriteBinaryFile(const std::wstring& path, const std::vector<uint8_t>& data) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, 
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to open file for writing: " + StringUtils::WideToUtf8(path) +
                 ", error: " + std::to_string(GetLastError()));
        return false;
    }

    DWORD bytesWritten;
    BOOL success = WriteFile(hFile, data.data(), (DWORD)data.size(), &bytesWritten, NULL);
    CloseHandle(hFile);

    if (!success || bytesWritten != data.size()) {
        LOG_ERROR("Failed to write file: " + StringUtils::WideToUtf8(path) +
                 ", error: " + std::to_string(GetLastError()));
        return false;
    }

    return true;
}

std::wstring FileUtils::GetDirectoryPath(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L"";
    return path.substr(0, pos);
}

std::wstring FileUtils::GetFileName(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return path;
    return path.substr(pos + 1);
}

bool FileUtils::SetFileHidden(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    
    return SetFileAttributesW(path.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN);
}

bool FileUtils::SetFileReadOnly(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    
    return SetFileAttributesW(path.c_str(), attrs | FILE_ATTRIBUTE_READONLY);
}

uint64_t FileUtils::GetFileModifiedTime(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) {
        return 0;
    }
    
    ULARGE_INTEGER time;
    time.HighPart = fad.ftLastWriteTime.dwHighDateTime;
    time.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    return time.QuadPart;
}

bool FileUtils::IsFileSigned(const std::wstring& path) {
    // Simplified signature check - in real implementation would use WinVerifyTrust
    return FileExists(path); // Placeholder
}