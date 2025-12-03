#pragma once

#include <array>
#include <string>

namespace game::minigame
{
    struct PatternMatchingState
    {
        enum class Phase
        {
            Inactive,
            ShowingPattern,
            WaitingInput,
            Success,
            Failure
        };
        
        Phase phase = Phase::Inactive;
        std::array<int, 4> pattern{};
        std::array<int, 4> player_input{};
        int input_index = 0;
        float show_timer = 0.0f;
        float show_duration = 3.0f;
        float input_timer = 0.0f;
        float input_time_limit = 5.0f;
        bool success = false;
        std::string display_text;
        int bonus_steps = 0;
    };

    void start_pattern_matching(PatternMatchingState& state);
    void advance(PatternMatchingState& state, float delta_time);
    void submit_input(PatternMatchingState& state, int direction);
    bool is_running(const PatternMatchingState& state);
    bool is_success(const PatternMatchingState& state);
    bool is_failure(const PatternMatchingState& state);
    std::string get_display_text(const PatternMatchingState& state);
    int get_bonus_steps(const PatternMatchingState& state);
    void reset(PatternMatchingState& state);
}



