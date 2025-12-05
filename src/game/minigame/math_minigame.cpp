#include "math_minigame.h"

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

        std::uniform_int_distribution<int>& get_number_distribution()
        {
            static std::uniform_int_distribution<int> num_dist(1, 20);
            return num_dist;
        }
    }

    void start_math_quiz(MathQuizState& state)
    {
        state.num1 = get_number_distribution()(get_rng());
        state.num2 = get_number_distribution()(get_rng());
        state.operation = '+';
        state.correct_answer = state.num1 + state.num2;
        state.player_answer = 0;
        state.title_timer = 0.0f;
        state.title_duration = 3.0f;
        state.timer = 0.0f;
        state.time_limit = 15.0f;
        state.phase = MathQuizState::Phase::ShowingTitle;
        state.success = false;
        state.bonus_steps = 0;
        state.input_buffer.clear();
        state.display_text = "Math Quiz";
    }

    void advance(MathQuizState& state, float delta_time)
    {
        if (state.phase == MathQuizState::Phase::ShowingTitle)
        {
            state.title_timer += delta_time;
            if (state.title_timer >= state.title_duration)
            {
                state.phase = MathQuizState::Phase::ShowingQuestion;
                state.timer = 0.0f;
            }
        }
        else if (state.phase == MathQuizState::Phase::ShowingQuestion)
        {
            state.timer += delta_time;
            if (state.timer >= 0.1f)  // Show question immediately after title
            {
                // After showing title, show the question
                state.phase = MathQuizState::Phase::WaitingAnswer;
                state.timer = 0.0f;
                std::ostringstream oss;
                oss << state.num1 << " + " << state.num2 << " =";
                if (!state.input_buffer.empty())
                {
                    oss << " " << state.input_buffer;
                }
                else
                {
                    oss << "   ";
                }
                oss << " (T: " << static_cast<int>(state.time_limit) << "s)";
                state.display_text = oss.str();
            }
        }
        else if (state.phase == MathQuizState::Phase::WaitingAnswer)
        {
            state.timer += delta_time;
            if (state.timer >= state.time_limit)
            {
                state.phase = MathQuizState::Phase::Failure;
                state.success = false;
                state.bonus_steps = 0;
                state.display_text = "Time's Up!";
                state.timer = 0.0f;  // Reset timer for 2 second display
            }
            else
            {
                // Update display text with remaining time and input buffer
                float remaining_time = state.time_limit - state.timer;
                int remaining_seconds = static_cast<int>(remaining_time);
                std::ostringstream oss;
                oss << state.num1 << " + " << state.num2 << " =";
                if (!state.input_buffer.empty())
                {
                    oss << " " << state.input_buffer;
                }
                else
                {
                    oss << "   ";
                }
                oss << " (T: " << remaining_seconds << "s)";
                state.display_text = oss.str();
            }
        }
        else if (state.phase == MathQuizState::Phase::Success)
        {
            // Show success message for 2 seconds
            state.timer += delta_time;
            if (state.timer >= 2.0f)
            {
                // After 2 seconds, reset the game
                reset(state);
            }
        }
        else if (state.phase == MathQuizState::Phase::Failure)
        {
            // Show failure message for 2 seconds
            state.timer += delta_time;
            if (state.timer >= 2.0f)
            {
                // After 2 seconds, reset the game
                reset(state);
            }
        }
    }

    void submit_answer(MathQuizState& state, int answer)
    {
        if (state.phase != MathQuizState::Phase::WaitingAnswer)
        {
            return;
        }

        state.player_answer = answer;
        if (answer == state.correct_answer)
        {
            state.phase = MathQuizState::Phase::Success;
            state.success = true;
            state.bonus_steps = 4;
            state.display_text = "Correct! +4 steps";
            state.timer = 0.0f;  // Reset timer for 2 second display
        }
        else
        {
            state.phase = MathQuizState::Phase::Failure;
            state.success = false;
            state.bonus_steps = 0;
            state.display_text = "Wrong Answer!";
            state.timer = 0.0f;  // Reset timer for 2 second display
        }
    }

    bool is_running(const MathQuizState& state)
    {
        return state.phase == MathQuizState::Phase::ShowingTitle ||
               state.phase == MathQuizState::Phase::ShowingQuestion ||
               state.phase == MathQuizState::Phase::WaitingAnswer;
    }

    bool is_success(const MathQuizState& state)
    {
        return state.phase == MathQuizState::Phase::Success;
    }

    bool is_failure(const MathQuizState& state)
    {
        return state.phase == MathQuizState::Phase::Failure;
    }

    std::string get_display_text(const MathQuizState& state)
    {
        return state.display_text;
    }

    int get_bonus_steps(const MathQuizState& state)
    {
        return state.bonus_steps;
    }

    void add_digit(MathQuizState& state, char digit)
    {
        if (state.phase != MathQuizState::Phase::WaitingAnswer)
        {
            return;
        }
        if (state.input_buffer.size() < 3)  // Max 3 digits (up to 100)
        {
            state.input_buffer += digit;
        }
    }

    void remove_digit(MathQuizState& state)
    {
        if (state.phase != MathQuizState::Phase::WaitingAnswer)
        {
            return;
        }
        if (!state.input_buffer.empty())
        {
            state.input_buffer.pop_back();
        }
    }

    void submit_buffer(MathQuizState& state)
    {
        if (state.phase != MathQuizState::Phase::WaitingAnswer || state.input_buffer.empty())
        {
            return;
        }

        try
        {
            int answer = std::stoi(state.input_buffer);
            // Validate range (1-100)
            if (answer >= 1 && answer <= 100)
            {
                submit_answer(state, answer);
                state.input_buffer.clear();
            }
            else
            {
                // Invalid range, clear buffer
                state.input_buffer.clear();
            }
        }
        catch (const std::exception&)
        {
            // Invalid input, clear buffer
            state.input_buffer.clear();
        }
    }

    void reset(MathQuizState& state)
    {
        state.phase = MathQuizState::Phase::Inactive;
        state.title_timer = 0.0f;
        state.timer = 0.0f;
        state.success = false;
        state.display_text.clear();
        state.input_buffer.clear();
        state.bonus_steps = 0;
    }
}



