#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <vector>
#include <cstdint>

class FileUtils {
public:
    static std::wstring GetCurrentExecutablePath();
    static std::wstring GetCurrentModulePath();
    static std::wstring GetAppDataPath();
    static std::wstring GetTempPath();
    static std::wstring GetSystemPath();
    
    static bool FileExists(const std::wstring& path);
    static bool DirectoryExists(const std::wstring& path);
    static bool CreateDirectories(const std::wstring& path);
    
    static bool DeleteFile(const std::wstring& path);
    static bool MoveFile(const std::wstring& from, const std::wstring& to);
    static bool CopyFile(const std::wstring& from, const std::wstring& to);
    
    static uint64_t GetFileSize(const std::wstring& path);
    static std::vector<uint8_t> ReadBinaryFile(const std::wstring& path);
    static bool WriteBinaryFile(const std::wstring& path, const std::vector<uint8_t>& data);
    
    static std::wstring GetDirectoryPath(const std::wstring& path);
    static std::wstring GetFileName(const std::wstring& path);
    
    static bool SetFileHidden(const std::wstring& path);
    static bool SetFileReadOnly(const std::wstring& path);
    
    static uint64_t GetFileModifiedTime(const std::wstring& path);
    static bool IsFileSigned(const std::wstring& path);
};

#endif