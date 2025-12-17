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
            ShowingTitle,
            ShowingPattern,
            WaitingInput,
            Success,
            Failure
        };
        
        Phase phase = Phase::Inactive;
        std::array<int, 4> pattern{};
        std::string input_buffer;  // Text input buffer
        float title_timer = 0.0f;
        float title_duration = 5.0f;
        float show_timer = 0.0f;
        float show_duration = 10.0f;
        bool success = false;
        std::string display_text;
        int bonus_steps = 0;
    };

    void start_pattern_matching(PatternMatchingState& state);
    void advance(PatternMatchingState& state, float delta_time);
    void add_char_input(PatternMatchingState& state, char c);
    void delete_char(PatternMatchingState& state);
    void submit_answer(PatternMatchingState& state);
    bool is_running(const PatternMatchingState& state);
    bool is_success(const PatternMatchingState& state);
    bool is_failure(const PatternMatchingState& state);
    std::string get_display_text(const PatternMatchingState& state);
    int get_bonus_steps(const PatternMatchingState& state);
    void reset(PatternMatchingState& state);
}



