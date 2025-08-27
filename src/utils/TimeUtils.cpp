// src/utils/TimeUtils.cpp
#include "utils/TimeUtils.h"
#include <Windows.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdint>
#include <string>

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
    return ::GetTickCount64();
}

uint64_t TimeUtils::GetSystemUptime() {
    return ::GetTickCount64();
}

void TimeUtils::Sleep(uint64_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void TimeUtils::JitterSleep(uint64_t baseMs, double jitterFactor) {
    if (jitterFactor < 0) jitterFactor = 0;
    if (jitterFactor > 1) jitterFactor = 1;

    uint64_t jitter = static_cast<uint64_t>(baseMs * jitterFactor);
    uint64_t sleepTime = baseMs + (rand() % (jitter * 2 + 1)) - jitter;
    
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
    // Could implement NTP sync or similar in advanced version
}