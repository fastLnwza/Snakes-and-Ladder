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
        state.timer = 0.0f;
        state.time_limit = 5.0f;
        state.phase = MathQuizState::Phase::WaitingAnswer;
        state.success = false;
        state.bonus_steps = 0;
        
        std::ostringstream oss;
        oss << "Math: " << state.num1 << " + " << state.num2 << " = ? (1-9)";
        state.display_text = oss.str();
    }

    void advance(MathQuizState& state, float delta_time)
    {
        if (state.phase == MathQuizState::Phase::WaitingAnswer)
        {
            state.timer += delta_time;
            if (state.timer >= state.time_limit)
            {
                state.phase = MathQuizState::Phase::Failure;
                state.success = false;
                state.bonus_steps = 0;
                state.display_text = "Time's Up!";
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
        }
        else
        {
            state.phase = MathQuizState::Phase::Failure;
            state.success = false;
            state.bonus_steps = 0;
            state.display_text = "Wrong Answer!";
        }
    }

    bool is_running(const MathQuizState& state)
    {
        return state.phase == MathQuizState::Phase::WaitingAnswer;
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

    void reset(MathQuizState& state)
    {
        state.phase = MathQuizState::Phase::Inactive;
        state.timer = 0.0f;
        state.success = false;
        state.display_text.clear();
        state.bonus_steps = 0;
    }
}



