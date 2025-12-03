#include "reaction_minigame.h"

#include <chrono>
#include <random>

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

        std::uniform_real_distribution<float>& get_wait_distribution()
        {
            static std::uniform_real_distribution<float> wait_distribution(1.0f, 3.0f);
            return wait_distribution;
        }
    }

    void start_reaction(ReactionState& state)
    {
        state.phase = ReactionState::Phase::Waiting;
        state.wait_timer = 0.0f;
        state.ready_timer = 0.0f;
        state.wait_duration = get_wait_distribution()(get_rng());
        state.ready_duration = 2.0f;
        state.success = false;
        state.display_text = "Wait for GO!";
        state.bonus_steps = 0;
    }

    void advance(ReactionState& state, float delta_time)
    {
        switch (state.phase)
        {
        case ReactionState::Phase::Waiting:
            state.wait_timer += delta_time;
            if (state.wait_timer >= state.wait_duration)
            {
                state.phase = ReactionState::Phase::Ready;
                state.display_text = "GO! Press SPACE!";
            }
            break;
        case ReactionState::Phase::Ready:
            state.ready_timer += delta_time;
            if (state.ready_timer >= state.ready_duration)
            {
                state.phase = ReactionState::Phase::Failure;
                state.success = false;
                state.bonus_steps = 0;
                state.display_text = "Too Slow!";
            }
            break;
        default:
            break;
        }
    }

    void press_key(ReactionState& state)
    {
        if (state.phase == ReactionState::Phase::Ready)
        {
            state.phase = ReactionState::Phase::Success;
            state.success = true;
            state.bonus_steps = 3;
            state.display_text = "Perfect! +3 steps";
        }
        else if (state.phase == ReactionState::Phase::Waiting)
        {
            state.phase = ReactionState::Phase::Failure;
            state.success = false;
            state.bonus_steps = 0;
            state.display_text = "Too Early!";
        }
    }

    bool is_running(const ReactionState& state)
    {
        return state.phase == ReactionState::Phase::Waiting || 
               state.phase == ReactionState::Phase::Ready;
    }

    bool is_success(const ReactionState& state)
    {
        return state.phase == ReactionState::Phase::Success;
    }

    bool is_failure(const ReactionState& state)
    {
        return state.phase == ReactionState::Phase::Failure;
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
        state.wait_timer = 0.0f;
        state.ready_timer = 0.0f;
        state.success = false;
        state.display_text.clear();
        state.bonus_steps = 0;
    }
}


