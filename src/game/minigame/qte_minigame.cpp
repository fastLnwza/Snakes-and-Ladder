#include "qte_minigame.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace game::minigame
{
    namespace
    {
        constexpr float TIMER_SPEED_SCALE = 0.4f; // Slow the visual timer further

        std::string format_two_part_time(float time_seconds)
        {
            if (time_seconds < 0.0f)
            {
                time_seconds = 0.0f;
            }

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << time_seconds;
            return oss.str();
        }
    }

    void start_precision_timing(PrecisionTimingState& state)
    {
        state.status = PrecisionTimingStatus::Running;
        state.timer = 0.0f;
        state.stopped_time = 0.0f;
        state.target_time = 4.99f;
        state.max_time = 10.0f;
        state.bonus_steps = 0;
        state.display_text = "Press SPACE to stop at " + format_two_part_time(state.target_time) + "!";
    }

    void advance(PrecisionTimingState& state, float delta_time)
    {
        if (state.status == PrecisionTimingStatus::Running)
        {
            state.timer += delta_time * TIMER_SPEED_SCALE;
            std::string base = format_two_part_time(state.target_time) +
                               " : " + format_two_part_time(state.timer);
            state.display_text = base + "  SPACE!";
        }
        else if (state.is_showing_time)
        {
            // Count down the time display timer
            state.time_display_timer -= delta_time;
            
            // Update display text to show comparison: "4.99 : X.XX"
            std::ostringstream oss;
            oss << format_two_part_time(state.target_time) << " : " << format_two_part_time(state.stopped_time);
            state.display_text = oss.str();
            
            // After 0.5 seconds, switch to showing result
            if (state.time_display_timer <= 0.0f)
            {
                state.is_showing_time = false;
                state.display_text = state.result_message;
            }
        }
    }

    void stop_timing(PrecisionTimingState& state)
    {
        if (state.status != PrecisionTimingStatus::Running)
        {
            return;
        }

        state.stopped_time = state.timer;
        const float target = state.target_time;
        const float diff = std::abs(state.stopped_time - target);

        // Show the stopped time first for 0.5 seconds, then show result
        state.is_showing_time = true;
        state.time_display_timer = 1.5f;  // Show for longer to make results easier to read
        
        // Store result message but don't show it yet
        std::ostringstream result_oss;
        result_oss << std::fixed << std::setprecision(2);
        
        if (diff <= 0.01f) // Perfect: exactly 4.99 (within ±0.01)
        {
            state.status = PrecisionTimingStatus::Perfect;
            state.bonus_steps = 6; // Bonus +6 steps
            result_oss << "โบนัส +6";
        }
        else // Failure: not 4.99
        {
            state.status = PrecisionTimingStatus::Failure;
            state.bonus_steps = 0;
            result_oss << "Mission Fail";
        }

        state.result_message = result_oss.str();
        
        // Display comparison: "4.99 : X.XX" to show target vs actual time
        state.display_text = format_two_part_time(state.target_time) + " : " + format_two_part_time(state.stopped_time);
    }

    bool has_expired(const PrecisionTimingState& state)
    {
        return state.status == PrecisionTimingStatus::Running && state.timer >= state.max_time;
    }

    bool is_running(const PrecisionTimingState& state)
    {
        return state.status == PrecisionTimingStatus::Running;
    }

    bool is_success(const PrecisionTimingState& state)
    {
        return state.status == PrecisionTimingStatus::Perfect ||
               state.status == PrecisionTimingStatus::Good ||
               state.status == PrecisionTimingStatus::Ok;
    }

    bool is_failure(const PrecisionTimingState& state)
    {
        return state.status == PrecisionTimingStatus::Failure;
    }

    void reset(PrecisionTimingState& state)
    {
        state.status = PrecisionTimingStatus::Inactive;
        state.timer = 0.0f;
        state.stopped_time = 0.0f;
        state.display_text.clear();
        state.bonus_steps = 0;
        state.is_showing_time = false;
        state.time_display_timer = 0.0f;
        state.result_message.clear();
    }

    std::string get_display_text(const PrecisionTimingState& state)
    {
        return state.display_text;
    }

    int get_bonus_steps(const PrecisionTimingState& state)
    {
        return state.bonus_steps;
    }
}

