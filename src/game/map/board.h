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

    constexpr std::array<BoardLink, 12> BOARD_LINKS = {{
        // Ladders (3)
        {6, 17, {0.32f, 0.68f, 0.82f}, true},
        {20, 38, {0.46f, 0.78f, 0.36f}, true},
        {41, 62, {0.28f, 0.7f, 0.55f}, true},
        // Snakes (9) - เพิ่มงูให้เยอะขึ้น
        {16, 3, {0.78f, 0.24f, 0.24f}, false},   // Snake 1
        {47, 25, {0.82f, 0.3f, 0.18f}, false},   // Snake 2
        {54, 31, {0.9f, 0.2f, 0.2f}, false},      // Snake 3
        {64, 42, {0.85f, 0.25f, 0.15f}, false},  // Snake 4
        {71, 50, {0.88f, 0.22f, 0.18f}, false},  // Snake 5
        {79, 58, {0.8f, 0.3f, 0.2f}, false},     // Snake 6
        {87, 69, {0.9f, 0.25f, 0.2f}, false},    // Snake 7
        {92, 73, {0.85f, 0.28f, 0.22f}, false},  // Snake 8
        {96, 80, {0.88f, 0.2f, 0.15f}, false},   // Snake 9
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

