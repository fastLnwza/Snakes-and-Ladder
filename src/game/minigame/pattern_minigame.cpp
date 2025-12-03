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
        state.player_input.fill(0);
        state.input_index = 0;
        state.show_timer = 0.0f;
        state.show_duration = 3.0f;
        state.input_timer = 0.0f;
        state.input_time_limit = 5.0f;
        state.phase = PatternMatchingState::Phase::ShowingPattern;
        state.success = false;
        state.bonus_steps = 0;
        
        std::ostringstream oss;
        oss << "Pattern: ";
        for (int i = 0; i < 4; ++i)
        {
            const char* dirs[] = {"", "↑", "↓", "←", "→"};
            oss << dirs[state.pattern[i]] << " ";
        }
        state.display_text = oss.str();
    }

    void advance(PatternMatchingState& state, float delta_time)
    {
        switch (state.phase)
        {
        case PatternMatchingState::Phase::ShowingPattern:
            state.show_timer += delta_time;
            if (state.show_timer >= state.show_duration)
            {
                state.phase = PatternMatchingState::Phase::WaitingInput;
                state.input_timer = state.input_time_limit;
                state.display_text = "Repeat pattern! (WASD)";
            }
            break;
        case PatternMatchingState::Phase::WaitingInput:
            state.input_timer -= delta_time;
            if (state.input_timer <= 0.0f)
            {
                state.phase = PatternMatchingState::Phase::Failure;
                state.success = false;
                state.bonus_steps = 0;
                state.display_text = "Time's Up!";
            }
            break;
        default:
            break;
        }
    }

    void submit_input(PatternMatchingState& state, int direction)
    {
        if (state.phase != PatternMatchingState::Phase::WaitingInput)
        {
            return;
        }

        state.player_input[state.input_index] = direction;
        state.input_index++;

        // Check if pattern matches so far
        bool matches = true;
        for (int i = 0; i < state.input_index; ++i)
        {
            if (state.player_input[i] != state.pattern[i])
            {
                matches = false;
                break;
            }
        }

        if (!matches)
        {
            state.phase = PatternMatchingState::Phase::Failure;
            state.success = false;
            state.bonus_steps = 0;
            state.display_text = "Wrong Pattern!";
        }
        else if (state.input_index >= 4)
        {
            state.phase = PatternMatchingState::Phase::Success;
            state.success = true;
            state.bonus_steps = 5;
            state.display_text = "Perfect! +5 steps";
        }
    }

    bool is_running(const PatternMatchingState& state)
    {
        return state.phase == PatternMatchingState::Phase::ShowingPattern ||
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
        state.player_input.fill(0);
        state.input_index = 0;
        state.show_timer = 0.0f;
        state.input_timer = 0.0f;
        state.success = false;
        state.display_text.clear();
        state.bonus_steps = 0;
    }
}


