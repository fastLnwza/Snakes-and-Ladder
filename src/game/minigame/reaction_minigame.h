#pragma once

#include <string>

namespace game::minigame
{
    struct ReactionState
    {
        enum class Phase
        {
            Inactive,
            InitialMessage,  // Show "Guess 1-9" for 3 seconds
            PlayerTurn,      // Player guesses - showing input prompt
            ShowingGuess,    // Show the number player guessed for 2 seconds
            ShowingFeedback, // Show "Too low" / "Too high" for 1 second
            AITurn,          // AI guesses
            PlayerWon,
            AIWon,
            Failure
        };
        
        Phase phase = Phase::Inactive;
        float timer = 0.0f;
        float ai_thinking_time = 1.0f;     // Time AI takes to "think"
        float result_display_time = 1.5f;   // Time to show result
        
        int target_number = 0;               // Number to guess (1-9)
        int min_range = 1;
        int max_range = 9;
        
        int player_guess = 0;
        int ai_guess = 0;
        int ai_min = 1;
        int ai_max = 9;
        
        int player_attempts = 0;
        int ai_attempts = 0;
        int max_attempts = 3;               // Max attempts before losing
        
        bool success = false;
        std::string display_text;
        std::string last_feedback;          // "too low", "too high", "correct"
        std::string guessed_number_text;    // Store the guessed number for display
        int bonus_steps = 0;
    };

    void start_reaction(ReactionState& state);
    void advance(ReactionState& state, float delta_time);
    void submit_guess(ReactionState& state, int guess);
    bool is_running(const ReactionState& state);
    bool is_success(const ReactionState& state);
    bool is_failure(const ReactionState& state);
    std::string get_display_text(const ReactionState& state);
    int get_bonus_steps(const ReactionState& state);
    void reset(ReactionState& state);
}
