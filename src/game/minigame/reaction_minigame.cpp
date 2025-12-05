#include "reaction_minigame.h"

#include <chrono>
#include <random>
#include <sstream>
#include <algorithm>

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
            static std::uniform_int_distribution<int> number_distribution(1, 9);
            return number_distribution;
        }

        int get_ai_guess(int min_val, int max_val)
        {
            // AI uses binary search strategy
            return (min_val + max_val) / 2;
        }
    }

    void start_reaction(ReactionState& state)
    {
        state.phase = ReactionState::Phase::InitialMessage;
        state.timer = 0.0f;
        state.ai_thinking_time = 1.0f;
        state.result_display_time = 1.5f;
        
        // Generate random target number
        state.target_number = get_number_distribution()(get_rng());
        state.min_range = 1;
        state.max_range = 9;
        
        state.player_guess = 0;
        state.ai_guess = 0;
        state.ai_min = 1;
        state.ai_max = 9;
        
        state.player_attempts = 0;
        state.ai_attempts = 0;
        state.max_attempts = 3;
        
        state.success = false;
        state.display_text = "Guess 1-9";
        state.last_feedback.clear();
        state.guessed_number_text.clear();
        state.bonus_steps = 0;
    }

    void advance(ReactionState& state, float delta_time)
    {
        switch (state.phase)
        {
        case ReactionState::Phase::InitialMessage:
        {
            state.timer += delta_time;
            if (state.timer >= 3.0f)
            {
                // After 3 seconds, transition to PlayerTurn and show input prompt
                state.phase = ReactionState::Phase::PlayerTurn;
                state.timer = 0.0f;
                state.player_attempts = 0;
                std::stringstream ss;
                ss << "Guess " << (state.player_attempts + 1) << "/" << state.max_attempts << " : input";
                state.display_text = ss.str();
            }
            break;
        }
        case ReactionState::Phase::PlayerTurn:
        {
            // Player is waiting for input - nothing to do here
            break;
        }
        case ReactionState::Phase::ShowingGuess:
        {
            // Show the guessed number for 2 seconds
            state.timer += delta_time;
            if (state.timer >= 2.0f)
            {
                // After 2 seconds, check if this was the last attempt
                if (state.player_attempts >= state.max_attempts)
                {
                    // Last attempt - show Mission failed directly, skip feedback
                    state.phase = ReactionState::Phase::Failure;
                    state.success = false;
                    state.bonus_steps = 0;
                    state.display_text = "Mission failed";
                    state.timer = 0.0f;  // Reset timer for 3 second display
                }
                else
                {
                    // Still have attempts - show feedback
                    state.phase = ReactionState::Phase::ShowingFeedback;
                    state.display_text = state.last_feedback;
                }
                state.timer = 0.0f;
            }
            break;
        }
        case ReactionState::Phase::ShowingFeedback:
        {
            // Show feedback for 1 second (only for attempts 1 and 2)
            state.timer += delta_time;
            if (state.timer >= 1.0f)
            {
                // After 1 second, show next guess prompt
                state.phase = ReactionState::Phase::PlayerTurn;
                std::stringstream ss;
                ss << "Guess " << (state.player_attempts + 1) << "/" << state.max_attempts << " : input";
                state.display_text = ss.str();
                state.last_feedback.clear();
                state.guessed_number_text.clear();
                state.timer = 0.0f;
            }
            break;
        }
        case ReactionState::Phase::AITurn:
        {
            state.timer += delta_time;
            if (state.timer >= state.ai_thinking_time)
            {
                // AI makes a guess using binary search
                state.ai_guess = get_ai_guess(state.ai_min, state.ai_max);
                state.ai_attempts++;
                
                std::stringstream ss;
                ss << "AI guesses: " << state.ai_guess;
                
                if (state.ai_guess == state.target_number)
                {
                    // AI wins!
                    state.phase = ReactionState::Phase::AIWon;
                    state.success = false;
                    state.bonus_steps = 0;
                    ss << " - CORRECT! AI wins!";
                    state.display_text = ss.str();
                }
                else if (state.ai_guess < state.target_number)
                {
                    state.ai_min = state.ai_guess + 1;
                    ss << " - Too low!";
                    state.last_feedback = "AI: Too low";
                    state.phase = ReactionState::Phase::PlayerTurn;
                    state.timer = 0.0f;
                }
                else
                {
                    state.ai_max = state.ai_guess - 1;
                    ss << " - Too high!";
                    state.last_feedback = "AI: Too high";
                    state.phase = ReactionState::Phase::PlayerTurn;
                    state.timer = 0.0f;
                }
                
                if (state.phase == ReactionState::Phase::AITurn)
                {
                    state.display_text = ss.str();
                }
                else
                {
                    std::stringstream player_ss;
                    player_ss << "Your turn! Range: " << state.min_range << "-" << state.max_range 
                              << " | Attempts: " << state.player_attempts << "/" << state.max_attempts;
                    state.display_text = player_ss.str();
                }
                
                // Check if AI ran out of attempts
                if (state.ai_attempts >= state.max_attempts && state.phase != ReactionState::Phase::AIWon)
                {
                    state.phase = ReactionState::Phase::PlayerWon;
                    state.success = true;
                    state.bonus_steps = 3;
                    state.display_text = "AI ran out of attempts! You win! +3 steps";
                }
            }
            break;
        }
        case ReactionState::Phase::Failure:
        {
            // Show "Mission failed" for 2 seconds
            state.timer += delta_time;
            if (state.timer >= 2.0f)
            {
                // After 2 seconds, reset the game
                reset(state);
            }
            break;
        }
        case ReactionState::Phase::PlayerWon:
        {
            // Show "You win! +3 steps" for 2 seconds
            state.timer += delta_time;
            if (state.timer >= 2.0f)
            {
                // After 2 seconds, reset the game
                reset(state);
            }
            break;
        }
        default:
            break;
        }
    }

    void submit_guess(ReactionState& state, int guess)
    {
        if (state.phase != ReactionState::Phase::PlayerTurn)
        {
            return;
        }
        
        if (guess < 1 || guess > 9)
        {
            std::stringstream ss;
            ss << "Guess " << (state.player_attempts + 1) << "/" << state.max_attempts 
               << " : input - Invalid! Range is 1-9";
            state.display_text = ss.str();
            return;
        }
        
        state.player_guess = guess;
        state.player_attempts++;
        
        if (guess == state.target_number)
        {
            // Player wins!
            state.phase = ReactionState::Phase::PlayerWon;
            state.success = true;
            state.bonus_steps = 3;
            state.display_text = "You win! +3 steps";
            state.timer = 0.0f;  // Reset timer for 2 second display
        }
        else
        {
            // Wrong guess - show the guessed number first, then check attempts
            std::stringstream guess_ss;
            guess_ss << "Guess " << state.player_attempts << "/" << state.max_attempts << " : input " << guess;
            state.guessed_number_text = std::to_string(guess);
            state.display_text = guess_ss.str();
            state.last_feedback = (guess < state.target_number) ? "Too low" : "Too high";
            
            // Show the guessed number first, then check if mission failed after feedback
            state.phase = ReactionState::Phase::ShowingGuess;
            state.timer = 0.0f;
        }
    }

    bool is_running(const ReactionState& state)
    {
        return state.phase == ReactionState::Phase::InitialMessage ||
               state.phase == ReactionState::Phase::PlayerTurn || 
               state.phase == ReactionState::Phase::ShowingGuess ||
               state.phase == ReactionState::Phase::ShowingFeedback ||
               state.phase == ReactionState::Phase::AITurn;
    }

    bool is_success(const ReactionState& state)
    {
        return state.phase == ReactionState::Phase::PlayerWon;
    }

    bool is_failure(const ReactionState& state)
    {
        return state.phase == ReactionState::Phase::Failure || 
               state.phase == ReactionState::Phase::AIWon;
    }

    std::string get_display_text(const ReactionState& state)
    {
        return state.display_text;
    }

    int get_bonus_steps(const ReactionState& state)
    {
        return state.bonus_steps;
    }

    void reset(ReactionState& state)
    {
        state.phase = ReactionState::Phase::Inactive;
        state.timer = 0.0f;
        state.target_number = 0;
        state.min_range = 1;
        state.max_range = 9;
        state.player_guess = 0;
        state.ai_guess = 0;
        state.ai_min = 1;
        state.ai_max = 9;
        state.player_attempts = 0;
        state.ai_attempts = 0;
        state.success = false;
        state.display_text.clear();
        state.last_feedback.clear();
        state.guessed_number_text.clear();
        state.bonus_steps = 0;
    }
}
