#include "utils/FileUtils.h"
#include "core/Logger.h"
#include <fstream>
#include <random>
#include <vector>
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <Windows.h>
#include <Shlobj.h>
#include <fileapi.h>

std::string utils::FileUtils::GetCurrentExecutablePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::string(buffer);
}

std::string utils::FileUtils::GetCurrentModulePath() {
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)GetCurrentModulePath, &hModule);
    
    char buffer[MAX_PATH];
    GetModuleFileNameA(hModule, buffer, MAX_PATH);
    return std::string(buffer);
}

std::string utils::FileUtils::GetAppDataPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return std::string(path);
    }
    return GetTempPath();
}

std::string utils::FileUtils::GetTempPath() {
    char buffer[MAX_PATH];
    ::GetTempPathA(MAX_PATH, buffer);
    return std::string(buffer);
}

std::string utils::FileUtils::GetSystemPath() {
    char buffer[MAX_PATH];
    GetSystemDirectoryA(buffer, MAX_PATH);
    return std::string(buffer);
}

bool utils::FileUtils::FileExists(const std::string& path) {
    struct stat statbuf{};
    return (stat(path.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode));
}

bool utils::FileUtils::DirectoryExists(const std::string& path) {
    struct stat statbuf{};
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
            if (!CreateDirectoryA(subdir.c_str(), NULL)) {
                DWORD err = GetLastError();
                if (err != ERROR_ALREADY_EXISTS) {
                    LOG_ERROR("Failed to create directory: " + subdir + ", error: " + std::to_string(err));
                    return false;
                }
            }
        }
    }

    // Create the final directory
    return CreateDirectoryA(dir.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool utils::FileUtils::DeleteFile(const std::string& path) {
    if (!FileExists(path)) return true;
    
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs & FILE_ATTRIBUTE_READONLY) {
        SetFileAttributesA(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }
    return ::DeleteFileA(path.c_str()) != 0;
}

bool utils::FileUtils::MoveFile(const std::string& from, const std::string& to) {
    return ::MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
}

bool utils::FileUtils::CopyFile(const std::string& from, const std::string& to) {
    return ::CopyFileA(from.c_str(), to.c_str(), FALSE) != 0;
}

uint64_t utils::FileUtils::GetFileSize(const std::string& path) {
    struct stat statbuf{};
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
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return SetFileAttributesA(path.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN);
}

bool utils::FileUtils::SetFileReadOnly(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return SetFileAttributesA(path.c_str(), attrs | FILE_ATTRIBUTE_READONLY);
}

uint64_t utils::FileUtils::GetFileModifiedTime(const std::string& path) {
    struct stat statbuf{};
    if (stat(path.c_str(), &statbuf) != 0) {
        return 0;
    }
    return static_cast<uint64_t>(statbuf.st_mtime);
}

bool utils::FileUtils::IsFileSigned(const std::string& path) {
    // Signature checking is complex, this is a placeholder
    return FileExists(path);
}

std::wstring utils::FileUtils::StringToWide(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string utils::FileUtils::WideToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}