#pragma once

// Timer.h - High-resolution frame timing.
//
// Uses std::chrono::steady_clock for monotonic time.
// Tick() returns the frame delta in seconds and keeps a rolling FPS average.

#include <chrono>
#include <deque>

class Timer
{
public:
    Timer() = default;

    void  Reset(); // Call once before the game loop starts
    float Tick();  // Advance one frame; returns delta-time in seconds

    float GetDeltaTime()  const { return m_dt;    }
    float GetTotalTime()  const { return m_total; }
    float GetFPS()        const { return m_fps;   }
    int   GetFrameCount() const { return m_frame; }

private:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    TimePoint m_startTime;
    TimePoint m_lastTime;

    float m_dt    = 0.016f;
    float m_total = 0.f;
    float m_fps   = 0.f;
    int   m_frame = 0;

    std::deque<float>       m_dtHistory;
    static constexpr int    kFPSWindowSize = 60;  // Frames in rolling average
};
