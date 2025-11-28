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

        std::string format_input_prompt(const TileMemoryState& state);
        std::string format_sequence_hint(const TileMemoryState& state);

        void begin_round(TileMemoryState& state, int sequence_length)
        {
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
            state.post_sequence_timer = state.post_sequence_delay;
            state.input_time_limit = 5.0f + static_cast<float>(sequence_length) * 0.75f;
            state.input_timer = state.input_time_limit;
            state.result_text.clear();
            state.result_timer = 0.0f;
            state.display_text = format_sequence_hint(state);
            state.pending_next_round = false;
        }

        void set_result(TileMemoryState& state, bool round_success)
        {
            state.phase = Phase::Result;
            state.result_timer = RESULT_DISPLAY_TIME;
            const bool has_next_round = round_success && (state.current_round + 1 < state.total_rounds);
            state.pending_next_round = has_next_round;

            if (has_next_round)
            {
                state.result_text = "MEM ROUND " + std::to_string(state.current_round + 1) + " OK";
            }
            else
            {
                state.success = round_success;
                state.bonus_steps = state.success ? 4 : 0;
                state.result_text = state.success ? "MEM SUCCESS +4" : "MEM FAIL";
            }

            state.display_text = state.result_text;
        }

        std::string format_input_prompt(const TileMemoryState& state)
        {
            std::ostringstream oss;
            oss << "MEM INPUT: ";
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
                return "MEM READY...";
            }

            std::ostringstream oss;
            oss << "MEM SHOW: ";
            for (std::size_t i = 0; i < state.sequence.size(); ++i)
            {
                const int index = static_cast<int>(i);
                const bool revealed = index <= state.highlight_index;

                if (revealed)
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
    }

    void start(TileMemoryState& state, int sequence_length)
    {
        const int first_length = clamp_length(sequence_length);
        const int second_length = clamp_length(sequence_length + 1);
        state.round_lengths = {first_length, second_length};
        state.total_rounds = static_cast<int>(state.round_lengths.size());
        state.current_round = 0;
        state.success = false;
        state.pending_next_round = false;
        state.bonus_steps = 0;

        begin_round(state, state.round_lengths[state.current_round]);
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
                state.highlight_index = static_cast<int>(state.sequence.size());
                state.phase = Phase::SequencePause;
                state.post_sequence_timer = state.post_sequence_delay;
                state.display_text = format_sequence_hint(state);
            }
            else
            {
                state.display_text = format_sequence_hint(state);
            }
            break;
        }
        case Phase::SequencePause:
        {
            state.post_sequence_timer -= delta_time;
            if (state.post_sequence_timer <= 0.0f)
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
                if (state.pending_next_round)
                {
                    state.current_round++;
                    state.pending_next_round = false;
                    begin_round(state, state.round_lengths[state.current_round]);
                }
                else
                {
                    reset(state);
                }
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
            set_result(state, true);
            return;
        }

        state.display_text = format_input_prompt(state);
    }

    bool is_running(const TileMemoryState& state)
    {
        return state.phase == Phase::ShowingSequence ||
               state.phase == Phase::SequencePause ||
               state.phase == Phase::WaitingInput;
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
        state.post_sequence_timer = 0.0f;
        state.input_timer = 0.0f;
        state.success = false;
        state.pending_next_round = false;
        state.display_text.clear();
        state.result_text.clear();
        state.result_timer = 0.0f;
        state.bonus_steps = 0;
        state.current_round = 0;
    }
}


