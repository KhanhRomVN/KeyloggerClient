#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace utils {
class FileUtils {
public:
    static std::string GetCurrentExecutablePath();
    static std::string GetCurrentModulePath();
    static std::string GetAppDataPath();
    static std::string GetTempPath();
    static std::string GetSystemPath();
    
    static bool FileExists(const std::string& path);
    static bool DirectoryExists(const std::string& path);
    static bool CreateDirectories(const std::string& path);
    static bool DeleteFile(const std::string& path);
    static bool MoveFile(const std::string& from, const std::string& to);
    static bool CopyFile(const std::string& from, const std::string& to);
    static uint64_t GetFileSize(const std::string& path);
    static std::vector<uint8_t> ReadBinaryFile(const std::string& path);
    static bool WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data);
    
    static std::string GetDirectoryPath(const std::string& path);
    static std::string GetFileName(const std::string& path);
    
    static bool SetFileHidden(const std::string& path);
    static bool SetFileReadOnly(const std::string& path);
    
    static uint64_t GetFileModifiedTime(const std::string& path);
    static bool IsFileSigned(const std::string& path);
    
    // Helper functions for path conversion
    static std::wstring StringToWide(const std::string& str);
    static std::string WideToString(const std::wstring& wstr);

    // Additional functions that were being called
    static std::string GetTempPathA();
    static bool DeleteFileA(const std::string& path);
    static bool MoveFileA(const std::string& from, const std::string& to);
    static bool CopyFileA(const std::string& from, const std::string& to);
};
}

#endif