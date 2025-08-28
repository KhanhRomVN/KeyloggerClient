// src/utils/TimeUtils.cpp
#include "utils/TimeUtils.h"
#include "core/Platform.h"
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdint>
#include <string>
#include <random>

#if PLATFORM_WINDOWS
#include <Windows.h>
#elif PLATFORM_LINUX
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#endif

namespace utils {

std::string TimeUtils::GetCurrentTimestamp(bool forFilename) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    if (forFilename) {
        ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
        ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    } else {
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    }
    return ss.str();
}

uint64_t TimeUtils::GetTickCount() {
#if PLATFORM_WINDOWS
    return ::GetTickCount64();
#elif PLATFORM_LINUX
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#endif
}

uint64_t TimeUtils::GetSystemUptime() {
#if PLATFORM_WINDOWS
    return ::GetTickCount64();
#elif PLATFORM_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.uptime * 1000; // Convert seconds to milliseconds
    }
    return 0;
#endif
}

void TimeUtils::Sleep(uint64_t milliseconds) {
#if PLATFORM_WINDOWS
    ::Sleep(milliseconds);
#elif PLATFORM_LINUX
    usleep(milliseconds * 1000); // usleep takes microseconds
#endif
}

void TimeUtils::JitterSleep(uint64_t baseMs, double jitterFactor) {
    if (jitterFactor < 0) jitterFactor = 0;
    if (jitterFactor > 1) jitterFactor = 1;
    
    // Use proper random number generation
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    uint64_t jitter = static_cast<uint64_t>(baseMs * jitterFactor);
    std::uniform_int_distribution<uint64_t> dist(
        baseMs > jitter ? baseMs - jitter : 0, 
        baseMs + jitter
    );
    
    uint64_t sleepTime = dist(gen);
    Sleep(sleepTime);
}

std::string TimeUtils::FormatDuration(uint64_t milliseconds) {
    uint64_t seconds = milliseconds / 1000;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    uint64_t days = hours / 24;
    
    std::stringstream ss;
    if (days > 0) ss << days << "d ";
    if (hours > 0) ss << hours % 24 << "h ";
    if (minutes > 0) ss << minutes % 60 << "m ";
    ss << seconds % 60 << "s ";
    ss << milliseconds % 1000 << "ms";
    
    return ss.str();
}

bool TimeUtils::IsTimeOddSecond() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    return (time % 2) == 1;
}

void TimeUtils::SyncWithSystemTime() {
    // Placeholder for future NTP sync implementation
    // Could use platform-specific time synchronization methods
#if PLATFORM_WINDOWS
    // Windows time sync could use w32tm or similar
#elif PLATFORM_LINUX
    // Linux could use ntpdate or chrony
#endif
}

} // namespace utils