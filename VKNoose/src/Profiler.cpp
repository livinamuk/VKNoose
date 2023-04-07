#include "Profiler.h"
#include <iostream>

std::vector<ProfilerRecord> Profiler::s_profilerRecords;

Profiler::Profiler(const char* name, bool log_to_console_on_desctruction)
{
    m_startTime = std::chrono::steady_clock::now();
    m_name = name;
    m_log_to_console_on_desctruction = log_to_console_on_desctruction;
}

Profiler::~Profiler() 
{
    //glFinish();
    std::chrono::duration<float> duration = std::chrono::steady_clock::now() - m_startTime;
    float time = duration.count() * 1000.0f;

    std::string spacing;
    for (int i = 0; i < (40 - std::string(m_name).length()); i++)
        spacing += " ";
 
    if (m_log_to_console_on_desctruction)
        std::cout << m_name << ": " << spacing << time << "ms\n";

    // First check if a profiler record with this same name already exists
    for (auto& record : s_profilerRecords)
    {
        // If so then add the new data to it
        if (record.m_name == m_name) {
            record.m_lastTime = time;
            record.m_total += time;
            record.m_count++;
            record.m_averageTime = record.m_total / record.m_count;

            // Reset every 1000 frames
            if (record.m_count > 60) {
                record.m_lastTime = time;
                record.m_total = time;
                record.m_count = 1;
                record.m_averageTime = record.m_total / record.m_count;
                //std::cout << "reste\n";
            }

            return;
        }
    }
    // If not then create one
    ProfilerRecord record;
    record.m_name = m_name;
    record.m_lastTime = time;
    record.m_total += time;
    record.m_count++;
    record.m_averageTime = record.m_total / record.m_count;
    s_profilerRecords.push_back(record);
}

float Profiler::GetAverageRecordTime(const char* name) 
{
    for (auto& record : s_profilerRecords)
        if (record.m_name == name)
            return record.m_averageTime;
    return 0;
}