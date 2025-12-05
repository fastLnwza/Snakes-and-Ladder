#include "pattern_minigame.h"

#include <chrono>
#include <random>
#include <sstream>

namespace game::minigame
{
    namespace
    {
        std::mt19937& get_rng()
        {
            static std::mt19937 rng(static_cast<unsigned int>(
                std::chrono::steady_clock::now().time_since_epoch().count()));
            return rng;
        }

        std::uniform_int_distribution<int>& get_direction_distribution()
        {
            static std::uniform_int_distribution<int> dir_dist(1, 4);  // 1=Up, 2=Down, 3=Left, 4=Right
            return dir_dist;
        }
    }

    void start_pattern_matching(PatternMatchingState& state)
    {
        // Generate random pattern
        for (int i = 0; i < 4; ++i)
        {
            state.pattern[i] = get_direction_distribution()(get_rng());
        }
        state.input_buffer.clear();
        state.title_timer = 0.0f;
        state.title_duration = 5.0f;
        state.show_timer = 0.0f;
        state.show_duration = 5.0f;
        state.phase = PatternMatchingState::Phase::ShowingTitle;
        state.success = false;
        state.bonus_steps = 5;  // Set bonus for display
        state.display_text = "Pattern Matching! Bonus +5";
    }

    void advance(PatternMatchingState& state, float delta_time)
    {
        switch (state.phase)
        {
        case PatternMatchingState::Phase::ShowingTitle:
        {
            state.title_timer += delta_time;
            if (state.title_timer >= state.title_duration)
            {
                state.phase = PatternMatchingState::Phase::ShowingPattern;
                state.show_timer = 0.0f;
                // Generate pattern text using key letters (W S A D) that match the input keys
                std::ostringstream oss;
                for (int i = 0; i < 4; ++i)
                {
                    const char* dirs[] = {"", "W", "S", "A", "D"};
                    oss << dirs[state.pattern[i]];
                    if (i < 3) oss << " ";
                }
                state.display_text = oss.str();
            }
            break;
        }
        case PatternMatchingState::Phase::ShowingPattern:
            state.show_timer += delta_time;
            if (state.show_timer >= state.show_duration)
            {
                state.phase = PatternMatchingState::Phase::WaitingInput;
                state.input_buffer.clear();
                state.display_text = "Input: ";
            }
            break;
        case PatternMatchingState::Phase::WaitingInput:
        {
            // Update display text to show current input
            std::string input_text = "Input: ";
            input_text += state.input_buffer;
            // Add underscores for remaining slots
            int remaining = 4 - static_cast<int>(state.input_buffer.length());
            for (int i = 0; i < remaining; ++i)
            {
                input_text += "_";
                if (i < remaining - 1) input_text += " ";
            }
            state.display_text = input_text;
            break;
        }
        default:
            break;
        }
    }

    void add_char_input(PatternMatchingState& state, char c)
    {
        if (state.phase != PatternMatchingState::Phase::WaitingInput)
        {
            return;
        }

        // Only accept W, S, A, D (case insensitive)
        char upper_c = (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
        if (upper_c == 'W' || upper_c == 'S' || upper_c == 'A' || upper_c == 'D')
        {
            if (state.input_buffer.length() < 4)
            {
                state.input_buffer += upper_c;
            }
        }
    }

    void delete_char(PatternMatchingState& state)
    {
        if (state.phase != PatternMatchingState::Phase::WaitingInput)
        {
            return;
        }

        if (!state.input_buffer.empty())
        {
            state.input_buffer.pop_back();
        }
    }

    void submit_answer(PatternMatchingState& state)
    {
        if (state.phase != PatternMatchingState::Phase::WaitingInput)
        {
            return;
        }

        if (state.input_buffer.length() != 4)
        {
            // Not enough characters
            return;
        }

        // Convert input buffer to pattern format (1=W, 2=S, 3=A, 4=D)
        std::array<int, 4> player_pattern{};
        for (size_t i = 0; i < state.input_buffer.length(); ++i)
        {
            char c = state.input_buffer[i];
            if (c == 'W') player_pattern[i] = 1;
            else if (c == 'S') player_pattern[i] = 2;
            else if (c == 'A') player_pattern[i] = 3;
            else if (c == 'D') player_pattern[i] = 4;
        }

        // Check if pattern matches
        bool matches = true;
        for (int i = 0; i < 4; ++i)
        {
            if (player_pattern[i] != state.pattern[i])
            {
                matches = false;
                break;
            }
        }

        if (matches)
        {
            state.phase = PatternMatchingState::Phase::Success;
            state.success = true;
            state.bonus_steps = 5;
            state.display_text = "Perfect! +5 steps";
        }
        else
        {
            state.phase = PatternMatchingState::Phase::Failure;
            state.success = false;
            state.bonus_steps = 0;
            state.display_text = "Wrong Pattern!";
        }
    }

    bool is_running(const PatternMatchingState& state)
    {
        return state.phase == PatternMatchingState::Phase::ShowingTitle ||
               state.phase == PatternMatchingState::Phase::ShowingPattern ||
               state.phase == PatternMatchingState::Phase::WaitingInput;
    }

    bool is_success(const PatternMatchingState& state)
    {
        return state.phase == PatternMatchingState::Phase::Success;
    }

    bool is_failure(const PatternMatchingState& state)
    {
        return state.phase == PatternMatchingState::Phase::Failure;
    }

    std::string get_display_text(const PatternMatchingState& state)
    {
        return state.display_text;
    }

    int get_bonus_steps(const PatternMatchingState& state)
    {
        return state.bonus_steps;
    }

    void reset(PatternMatchingState& state)
    {
        state.phase = PatternMatchingState::Phase::Inactive;
        state.pattern.fill(0);
        state.input_buffer.clear();
        state.title_timer = 0.0f;
        state.show_timer = 0.0f;
        state.success = false;
        state.display_text.clear();
        state.bonus_steps = 0;
    }
}



