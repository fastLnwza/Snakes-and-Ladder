#include "tile_memory_minigame.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <random>
#include <sstream>

namespace game::minigame::tile_memory
{
    namespace
    {
        constexpr int MIN_SEQUENCE_LENGTH = 3;
        constexpr int MAX_SEQUENCE_LENGTH = 5;
        constexpr float RESULT_DISPLAY_TIME = 2.0f;
        constexpr int MAX_TILE_VALUE = 9;

        std::mt19937& rng()
        {
            static std::mt19937 engine(std::random_device{}());
            return engine;
        }

        int clamp_length(int length)
        {
            return std::clamp(length, MIN_SEQUENCE_LENGTH, MAX_SEQUENCE_LENGTH);
        }

        void set_result(TileMemoryState& state, bool success)
        {
            state.phase = Phase::Result;
            state.success = success;
            state.result_timer = RESULT_DISPLAY_TIME;
            state.bonus_steps = success ? 4 : 0;
            state.result_text = success ? "MEM SUCCESS +4" : "MEM FAIL";
            state.display_text = state.result_text;
        }

        std::string format_input_prompt(const TileMemoryState& state)
        {
            std::ostringstream oss;
            oss << "input ";
            const std::size_t total = state.sequence.size();
            for (std::size_t i = 0; i < total; ++i)
            {
                if (i < state.input_history.size())
                {
                    oss << state.input_history[i];
                }
                else
                {
                    oss << "_";
                }
                if (i + 1 < total)
                {
                    oss << " ";
                }
            }
            return oss.str();
        }

        std::string format_sequence_hint(const TileMemoryState& state)
        {
            if (state.sequence.empty())
            {
                return "mem ready...";
            }

            std::ostringstream oss;
            oss << "mem ";
            for (std::size_t i = 0; i < state.sequence.size(); ++i)
            {
                if (static_cast<int>(i) <= state.highlight_index)
                {
                    oss << state.sequence[i];
                }
                else
                {
                    oss << "_";
                }

                if (i + 1 < state.sequence.size())
                {
                    oss << " ";
                }
            }
            return oss.str();
        }

        void start_round(TileMemoryState& state, int sequence_length)
        {
            sequence_length = clamp_length(sequence_length);

            std::array<int, MAX_TILE_VALUE> tiles{};
            for (int i = 0; i < MAX_TILE_VALUE; ++i)
            {
                tiles[i] = i + 1;
            }

            std::shuffle(tiles.begin(), tiles.end(), rng());

            state.sequence.assign(tiles.begin(), tiles.begin() + sequence_length);
            state.input_history.clear();
            state.phase = Phase::ShowingSequence;
            state.highlight_index = 0;
            state.highlight_timer = 0.0f;
            state.input_time_limit = 5.0f + static_cast<float>(sequence_length) * 0.75f;
            state.input_timer = state.input_time_limit;
            state.display_text = format_sequence_hint(state);
        }
    }

    void start(TileMemoryState& state, int sequence_length)
    {
        (void)sequence_length;  // Ignore parameter, we use fixed rounds
        state.current_round = 1;  // Start with round 1 (3 digits)
        state.success = false;
        state.result_text.clear();
        state.result_timer = 0.0f;
        state.bonus_steps = 0;
        start_round(state, 3);  // First round: 3 digits
    }

    void advance(TileMemoryState& state, float delta_time)
    {
        switch (state.phase)
        {
        case Phase::Inactive:
            break;
        case Phase::ShowingSequence:
        {
            if (state.sequence.empty())
            {
                state.phase = Phase::Inactive;
                break;
            }

            state.highlight_timer += delta_time;
            if (state.highlight_timer >= state.highlight_interval)
            {
                state.highlight_timer = 0.0f;
                state.highlight_index++;
            }

            if (state.highlight_index >= static_cast<int>(state.sequence.size()))
            {
                state.phase = Phase::WaitingInput;
                state.input_timer = state.input_time_limit;
                state.display_text = format_input_prompt(state);
            }
            else
            {
                state.display_text = format_sequence_hint(state);
            }
            break;
        }
        case Phase::WaitingInput:
        {
            state.input_timer -= delta_time;
            if (state.input_timer <= 0.0f)
            {
                set_result(state, false);
            }
            else
            {
                state.display_text = format_input_prompt(state);
            }
            break;
        }
        case Phase::Result:
        {
            state.result_timer -= delta_time;
            if (state.result_timer <= 0.0f)
            {
                reset(state);
            }
            break;
        }
        }
    }

    void submit_choice(TileMemoryState& state, int tile_choice)
    {
        if (state.phase != Phase::WaitingInput || tile_choice < 1 || tile_choice > MAX_TILE_VALUE)
        {
            return;
        }

        state.input_history.push_back(tile_choice);
        const std::size_t current_index = state.input_history.size() - 1;
        if (state.sequence[current_index] != tile_choice)
        {
            set_result(state, false);
            return;
        }

        if (state.input_history.size() >= state.sequence.size())
        {
            // Check if we completed a round
            if (state.current_round == 1)
            {
                // Round 1 complete, start round 2 (4 digits)
                state.current_round = 2;
                start_round(state, 4);  // Second round: 4 digits
                return;
            }
            else
            {
                // Round 2 complete, game success
                set_result(state, true);
                return;
            }
        }

        state.display_text = format_input_prompt(state);
    }

    bool is_running(const TileMemoryState& state)
    {
        return state.phase == Phase::ShowingSequence || state.phase == Phase::WaitingInput;
    }

    bool is_active(const TileMemoryState& state)
    {
        return state.phase != Phase::Inactive;
    }

    bool is_result(const TileMemoryState& state)
    {
        return state.phase == Phase::Result;
    }

    bool is_success(const TileMemoryState& state)
    {
        return state.success && state.phase == Phase::Result;
    }

    std::string get_display_text(const TileMemoryState& state)
    {
        return state.display_text;
    }

    int get_bonus_steps(const TileMemoryState& state)
    {
        return state.bonus_steps;
    }

    void reset(TileMemoryState& state)
    {
        state.phase = Phase::Inactive;
        state.sequence.clear();
        state.input_history.clear();
        state.highlight_index = 0;
        state.highlight_timer = 0.0f;
        state.input_timer = 0.0f;
        state.success = false;
        state.display_text.clear();
        state.result_text.clear();
        state.result_timer = 0.0f;
        state.bonus_steps = 0;
        state.current_round = 0;
    }
}


