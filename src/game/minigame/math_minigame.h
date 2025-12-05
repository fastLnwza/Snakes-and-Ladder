#pragma once

#include <string>

namespace game::minigame
{
    struct MathQuizState
    {
        enum class Phase
        {
            Inactive,
            ShowingTitle,
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
        float title_timer = 0.0f;
        float title_duration = 3.0f;
        float timer = 0.0f;
        float time_limit = 15.0f;
        bool success = false;
        std::string display_text;
        std::string input_buffer;  // Buffer for multi-digit input
        int bonus_steps = 0;
    };

    void start_math_quiz(MathQuizState& state);
    void advance(MathQuizState& state, float delta_time);
    void submit_answer(MathQuizState& state, int answer);
    void add_digit(MathQuizState& state, char digit);
    void remove_digit(MathQuizState& state);
    void submit_buffer(MathQuizState& state);
    bool is_running(const MathQuizState& state);
    bool is_success(const MathQuizState& state);
    bool is_failure(const MathQuizState& state);
    std::string get_display_text(const MathQuizState& state);
    int get_bonus_steps(const MathQuizState& state);
    void reset(MathQuizState& state);
}



