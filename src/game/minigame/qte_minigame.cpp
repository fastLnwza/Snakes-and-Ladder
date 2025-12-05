#include "qte_minigame.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace game::minigame
{
    void start_precision_timing(PrecisionTimingState& state)
    {
        state.status = PrecisionTimingStatus::ShowingTitle;
        state.title_timer = 0.0f;
        state.title_duration = 3.0f;
        state.timer = 0.0f;
        state.stopped_time = 0.0f;
        state.target_time = 4.99f;
        state.max_time = 10.0f;
        state.bonus_steps = 0;
        state.display_text = "Precision Timing Game";
    }

    void advance(PrecisionTimingState& state, float delta_time)
    {
        if (state.status == PrecisionTimingStatus::ShowingTitle)
        {
            state.title_timer += delta_time;
            if (state.title_timer >= state.title_duration)
            {
                state.status = PrecisionTimingStatus::Running;
                state.display_text = "Press SPACE to stop at 4.99!";
            }
        }
        else if (state.status == PrecisionTimingStatus::Running)
        {
            // Slow down timer by 50% to make it easier
            state.timer += delta_time * 0.5f;
            // Update display text to show current time
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "4.99:" << state.timer << " [space!]";
            state.display_text = oss.str();
        }
        else if (state.is_showing_time)
        {
            // Update display text to show comparison: "4.99 : X.XX"
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "4.99 : " << state.stopped_time;
            state.display_text = oss.str();
            
            // Count down the time display timer
            state.time_display_timer -= delta_time;
            
            // After 0.5 seconds, switch to showing result
            if (state.time_display_timer <= 0.0f)
            {
                state.is_showing_time = false;
                state.display_text = state.result_message;
            }
        }
        else if (!state.is_showing_time && 
                 (state.status == PrecisionTimingStatus::Perfect || 
                  state.status == PrecisionTimingStatus::Failure))
        {
            // Ensure display_text is set to result_message when not showing time
            state.display_text = state.result_message;
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
        state.time_display_timer = 0.5f;  // Show for 0.5 seconds
        
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
        std::ostringstream time_oss;
        time_oss << std::fixed << std::setprecision(2);
        time_oss << "4.99 : " << state.stopped_time;
        state.display_text = time_oss.str();
    }

    bool has_expired(const PrecisionTimingState& state)
    {
        return state.status == PrecisionTimingStatus::Running && state.timer >= state.max_time;
    }

    bool is_running(const PrecisionTimingState& state)
    {
        return state.status == PrecisionTimingStatus::ShowingTitle ||
               state.status == PrecisionTimingStatus::Running;
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
        state.title_timer = 0.0f;
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

