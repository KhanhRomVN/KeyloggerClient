#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <string>
#include <cstdint>

class TimeUtils {
public:
    static std::string GetCurrentTimestamp(bool forFilename = false);
    static uint64_t GetTickCount();
    static uint64_t GetSystemUptime();
    static void Sleep(uint64_t milliseconds);
    static void JitterSleep(uint64_t baseMs, double jitterFactor);
    static std::string FormatDuration(uint64_t milliseconds);
    static bool IsTimeOddSecond();
    static void SyncWithSystemTime();
};

#endif