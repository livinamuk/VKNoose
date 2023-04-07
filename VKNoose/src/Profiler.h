#pragma once
#include <chrono>
#include <vector>

struct ProfilerRecord {
    std::string m_name;
    float m_lastTime = 0;
    float m_total = 0;
    float m_count = 0;
    float m_averageTime = 0;
};

struct Profiler
{
    // methods
    Profiler(const char* name, bool log_to_console_on_desctruction = false);
    ~Profiler();

    // member variables
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;// , m_endTime;
    //std::chrono::duration<float> m_duration;
    const char* m_name;
    bool m_log_to_console_on_desctruction;

    // static varibles
    static std::vector<ProfilerRecord> s_profilerRecords;

    // static functions
    static float GetAverageRecordTime(const char* name);
};