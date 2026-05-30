#pragma once

// ============================================================
//  Timer.h  -  High-Resolution Frame Timing
// ============================================================
//  Uses std::chrono::steady_clock for monotonic, high-res time.
//  Tick() returns delta-time in seconds and accumulates stats
//  for an FPS counter shown in the editor overlay.
// ============================================================

#include <chrono>
#include <deque>

class Timer
{
public:
    Timer() = default;

    /// Reset the clock (call before the game loop starts).
    void Reset();

    /// Advance one frame. Returns delta-time in seconds.
    float Tick();

    // ── Queries ───────────────────────────────────────────────
    float GetDeltaTime()    const { return m_dt; }     ///< Last frame dt (s)
    float GetTotalTime()    const { return m_total; }  ///< Elapsed since Reset (s)
    float GetFPS()          const { return m_fps; }    ///< Smoothed FPS
    int   GetFrameCount()   const { return m_frame; }  ///< Total frames since Reset

private:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    TimePoint m_startTime;
    TimePoint m_lastTime;

    float m_dt    = 0.016f;   // ~60 fps default
    float m_total = 0.f;
    float m_fps   = 0.f;
    int   m_frame = 0;

    // Rolling window for smoothed FPS (last N frames)
    std::deque<float> m_dtHistory;
    static constexpr int kFPSWindowSize = 60;
};
