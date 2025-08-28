#include "utils/FileUtils.h"
#include "core/Logger.h"
#include "security/Obfuscation.h"
#include "core/Platform.h"
#include <fstream>
#include <random>
#include <chrono>
#include <vector>
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <Shlobj.h>
#endif

// Obfuscated strings
/**
 *
 */
const auto OBF_FILE_UTILS = OBFUSCATE("FileUtils");

std::string utils::FileUtils::GetCurrentExecutablePath() {
#if PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::string(buffer);
#elif PLATFORM_LINUX
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::string(buffer);
    }
    return "";
#endif
}

std::string utils::FileUtils::GetCurrentModulePath() {
#if PLATFORM_WINDOWS
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)GetCurrentModulePath, &hModule);
    
    char buffer[MAX_PATH];
    GetModuleFileNameA(hModule, buffer, MAX_PATH);
    return std::string(buffer);
#elif PLATFORM_LINUX
    // On Linux, typically the same as executable path
    return GetCurrentExecutablePath();
#endif
}

std::string utils::FileUtils::GetAppDataPath() {
#if PLATFORM_WINDOWS
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::string(path);
    }
    return GetTempPath();
#elif PLATFORM_LINUX
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share";
    }
    return GetTempPath();
#endif
}

std::string utils::FileUtils::GetTempPath() {
#if PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    ::GetTempPathA(MAX_PATH, buffer);
    return std::string(buffer);
#elif PLATFORM_LINUX
    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = getenv("TEMP");
    if (!tmp) tmp = "/tmp";
    return std::string(tmp);
#endif
}

std::string utils::FileUtils::GetSystemPath() {
#if PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    GetSystemDirectoryA(buffer, MAX_PATH);
    return std::string(buffer);
#elif PLATFORM_LINUX
    return "/usr/bin"; // Common system path on Linux
#endif
}

bool utils::FileUtils::FileExists(const std::string& path) {
    struct stat statbuf;
    return (stat(path.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode));
}

bool utils::FileUtils::DirectoryExists(const std::string& path) {
    struct stat statbuf;
    return (stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

bool utils::FileUtils::CreateDirectories(const std::string& path) {
    if (DirectoryExists(path)) return true;

    std::string dir = path;
    if (dir.empty()) return false;
    
    // Remove trailing slash
    if (dir[dir.size() - 1] == '/' || dir[dir.size() - 1] == '\\') {
        dir.pop_back();
    }

    // Create parent directories first
    size_t pos = 0;
    while ((pos = dir.find_first_of("/\\", pos + 1)) != std::string::npos) {
        std::string subdir = dir.substr(0, pos);
        if (!DirectoryExists(subdir)) {
#if PLATFORM_WINDOWS
            if (!CreateDirectoryA(subdir.c_str(), NULL)) {
                DWORD err = GetLastError();
                if (err != ERROR_ALREADY_EXISTS) {
                    LOG_ERROR("Failed to create directory: " + subdir + ", error: " + std::to_string(err));
                    return false;
                }
            }
#elif PLATFORM_LINUX
            if (mkdir(subdir.c_str(), 0755) != 0) {
                if (errno != EEXIST) {
                    LOG_ERROR("Failed to create directory: " + subdir + ", error: " + std::to_string(errno));
                    return false;
                }
            }
#endif
        }
    }

    // Create the final directory
#if PLATFORM_WINDOWS
    return CreateDirectoryA(dir.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#elif PLATFORM_LINUX
    return mkdir(dir.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

bool utils::FileUtils::DeleteFile(const std::string& path) {
    if (!FileExists(path)) return true;
    
#if PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs & FILE_ATTRIBUTE_READONLY) {
        SetFileAttributesA(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }
    return ::DeleteFileA(path.c_str()) != 0;
#elif PLATFORM_LINUX
    return ::remove(path.c_str()) == 0;
#endif
}

bool utils::FileUtils::MoveFile(const std::string& from, const std::string& to) {
#if PLATFORM_WINDOWS
    return ::MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#elif PLATFORM_LINUX
    return ::rename(from.c_str(), to.c_str()) == 0;
#endif
}

bool utils::FileUtils::CopyFile(const std::string& from, const std::string& to) {
#if PLATFORM_WINDOWS
    return ::CopyFileA(from.c_str(), to.c_str(), FALSE) != 0;
#elif PLATFORM_LINUX
    std::ifstream src(from, std::ios::binary);
    std::ofstream dst(to, std::ios::binary);
    
    if (!src.is_open() || !dst.is_open()) {
        return false;
    }
    
    dst << src.rdbuf();
    return src.good() && dst.good();
#endif
}

uint64_t utils::FileUtils::GetFileSize(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) {
        return 0;
    }
    return static_cast<uint64_t>(statbuf.st_size);
}

std::vector<uint8_t> utils::FileUtils::ReadBinaryFile(const std::string& path) {
    std::vector<uint8_t> data;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for reading: " + path);
        return data;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    data.resize(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        LOG_ERROR("Failed to read file: " + path);
        data.clear();
    }

    return data;
}

bool utils::FileUtils::WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for writing: " + path);
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    bool success = file.good();
    
    if (!success) {
        LOG_ERROR("Failed to write file: " + path);
    }

    return success;
}

std::string utils::FileUtils::GetDirectoryPath(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return "";
    return path.substr(0, pos);
}

std::string utils::FileUtils::GetFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

bool utils::FileUtils::SetFileHidden(const std::string& path) {
#if PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return SetFileAttributesA(path.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN);
#elif PLATFORM_LINUX
    // On Linux, hidden files start with a dot
    std::string dir = GetDirectoryPath(path);
    std::string filename = GetFileName(path);
    
    if (filename.empty() || filename[0] == '.') {
        return true; // Already hidden
    }
    
    std::string newPath = dir + "/." + filename;
    return MoveFile(path, newPath);
#endif
}

bool utils::FileUtils::SetFileReadOnly(const std::string& path) {
#if PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return SetFileAttributesA(path.c_str(), attrs | FILE_ATTRIBUTE_READONLY);
#elif PLATFORM_LINUX
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) {
        return false;
    }
    return chmod(path.c_str(), statbuf.st_mode & ~(S_IWUSR | S_IWGRP | S_IWOTH)) == 0;
#endif
}

uint64_t utils::FileUtils::GetFileModifiedTime(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) {
        return 0;
    }
    return static_cast<uint64_t>(statbuf.st_mtime);
}

bool utils::FileUtils::IsFileSigned(const std::string& path) {
    // Signature checking is platform-specific and complex
    // For cross-platform compatibility, this is a placeholder
    return FileExists(path);
}

std::wstring utils::FileUtils::StringToWide(const std::string& str) {
#if PLATFORM_WINDOWS
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
#elif PLATFORM_LINUX
    // Simple conversion for Linux (may not handle all Unicode properly)
    return std::wstring(str.begin(), str.end());
#endif
}

std::string utils::FileUtils::WideToString(const std::wstring& wstr) {
#if PLATFORM_WINDOWS
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
#elif PLATFORM_LINUX
    // Simple conversion for Linux
    return std::string(wstr.begin(), wstr.end());
#endif
}