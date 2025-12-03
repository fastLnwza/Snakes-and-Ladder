#pragma once

#include <string>

namespace game::minigame
{
    struct ReactionState
    {
        enum class Phase
        {
            Inactive,
            Waiting,
            Ready,
            Success,
            Failure
        };
        
        Phase phase = Phase::Inactive;
        float wait_timer = 0.0f;
        float ready_timer = 0.0f;
        float wait_duration = 0.0f;
        float ready_duration = 2.0f;
        bool success = false;
        std::string display_text;
        int bonus_steps = 0;
    };

    void start_reaction(ReactionState& state);
    void advance(ReactionState& state, float delta_time);
    void press_key(ReactionState& state);
    bool is_running(const ReactionState& state);
    bool is_success(const ReactionState& state);
    bool is_failure(const ReactionState& state);
    std::string get_display_text(const ReactionState& state);
    int get_bonus_steps(const ReactionState& state);
    void reset(ReactionState& state);
}


