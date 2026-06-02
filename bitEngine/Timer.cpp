// Timer.cpp - High-resolution frame timing.

#include "Timer.h"
#include <numeric>

void Timer::Reset()
{
    m_startTime = Clock::now();
    m_lastTime  = m_startTime;
    m_dt        = 0.f;
    m_total     = 0.f;
    m_fps       = 0.f;
    m_frame     = 0;
    m_dtHistory.clear();
}

float Timer::Tick()
{
    auto now = Clock::now();

    m_dt    = std::chrono::duration<float>(now - m_lastTime).count();
    m_total = std::chrono::duration<float>(now - m_startTime).count();
    m_lastTime = now;

    ++m_frame;

    // Rolling FPS average over the last kFPSWindowSize frames.
    m_dtHistory.push_back(m_dt);
    if (static_cast<int>(m_dtHistory.size()) > kFPSWindowSize)
        m_dtHistory.pop_front();

    float avgDt = std::accumulate(m_dtHistory.begin(), m_dtHistory.end(), 0.f)
                  / static_cast<float>(m_dtHistory.size());

    m_fps = (avgDt > 0.f) ? 1.0f / avgDt : 0.f;

    return m_dt;
}
