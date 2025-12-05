#pragma once

#include <GLFW/glfw3.h>

#include <string>

namespace game::minigame
{
    enum class PrecisionTimingStatus
    {
        Inactive,
        ShowingTitle,
        Running,
        Perfect,
        Good,
        Ok,
        Failure
    };

    struct PrecisionTimingState
    {
        PrecisionTimingStatus status = PrecisionTimingStatus::Inactive;
        float title_timer = 0.0f;
        float title_duration = 3.0f;
        float timer = 0.0f;
        float stopped_time = 0.0f;
        float target_time = 4.99f;
        float max_time = 10.0f;
        std::string display_text;
        int bonus_steps = 0;
        bool is_showing_time = false;  // Show the stopped time before showing result
        float time_display_timer = 0.0f;  // Timer for showing the stopped time (5 seconds)
        std::string result_message;  // Store result message (Mission Fail or โบนัส +6)
    };

    void start_precision_timing(PrecisionTimingState& state);
    void advance(PrecisionTimingState& state, float delta_time);
    void stop_timing(PrecisionTimingState& state);
    bool has_expired(const PrecisionTimingState& state);
    bool is_running(const PrecisionTimingState& state);
    bool is_success(const PrecisionTimingState& state);
    bool is_failure(const PrecisionTimingState& state);
    void reset(PrecisionTimingState& state);
    std::string get_display_text(const PrecisionTimingState& state);
    int get_bonus_steps(const PrecisionTimingState& state);
}

