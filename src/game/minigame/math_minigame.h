#pragma once

#include <string>

namespace game::minigame
{
    struct MathQuizState
    {
        enum class Phase
        {
            Inactive,
            ShowingQuestion,
            WaitingAnswer,
            Success,
            Failure
        };
        
        Phase phase = Phase::Inactive;
        int num1 = 0;
        int num2 = 0;
        char operation = '+';
        int correct_answer = 0;
        int player_answer = 0;
        float timer = 0.0f;
        float time_limit = 5.0f;
        bool success = false;
        std::string display_text;
        int bonus_steps = 0;
    };

    void start_math_quiz(MathQuizState& state);
    void advance(MathQuizState& state, float delta_time);
    void submit_answer(MathQuizState& state, int answer);
    bool is_running(const MathQuizState& state);
    bool is_success(const MathQuizState& state);
    bool is_failure(const MathQuizState& state);
    std::string get_display_text(const MathQuizState& state);
    int get_bonus_steps(const MathQuizState& state);
    void reset(MathQuizState& state);
}


