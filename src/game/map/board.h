#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>

namespace game::map
{
    constexpr int BOARD_COLUMNS = 10;
    constexpr int BOARD_ROWS = 10;
    constexpr float TILE_SIZE = 7.0f;  // Increased to make map and board bigger

    enum class TileKind
    {
        Normal,
        Start,
        Finish,
        LadderBase,
        SnakeHead
    };

    struct BoardLink
    {
        int start = 0;
        int end = 0;
        glm::vec3 color{};
        bool is_ladder = false;
    };

    constexpr std::array<BoardLink, 5> BOARD_LINKS = {{
        {6, 17, {0.32f, 0.68f, 0.82f}, true},
        {20, 38, {0.46f, 0.78f, 0.36f}, true},
        {41, 62, {0.28f, 0.7f, 0.55f}, true},
        {16, 3, {0.78f, 0.24f, 0.24f}, false},
        {47, 25, {0.82f, 0.3f, 0.18f}, false},
    }};

    enum class ActivityKind
    {
        None,
        Bonus,
        Slide,
        Portal,
        Trap,
        MiniGame,
        MemoryGame,
        ReactionGame,
        MathGame,
        PatternGame,
        SkipTurn,
        WalkBackward
    };

    glm::vec3 tile_center_world(int tile_index, float height_offset = 0.0f);
    ActivityKind classify_activity_tile(int tile_index);
    bool check_wall_collision(const glm::vec3& position, float radius);
}

