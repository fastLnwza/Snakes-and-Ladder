#include "board.h"

#include <algorithm>
#include <cmath>

namespace game::map
{
    glm::vec3 tile_center_world(int tile_index, float height_offset)
    {
        const int row = tile_index / BOARD_COLUMNS;
        const int column_in_row = tile_index % BOARD_COLUMNS;
        const int column = (row % 2 == 0) ? column_in_row : (BOARD_COLUMNS - 1 - column_in_row);

        const float board_width = static_cast<float>(BOARD_COLUMNS) * TILE_SIZE;
        const float board_height = static_cast<float>(BOARD_ROWS) * TILE_SIZE;
        const float start_x = -board_width * 0.5f + TILE_SIZE * 0.5f;
        const float start_z = -board_height * 0.5f + TILE_SIZE * 0.5f;

        const float x = start_x + static_cast<float>(column) * TILE_SIZE;
        const float z = start_z + static_cast<float>(row) * TILE_SIZE;
        return {x, height_offset, z};
    }

    ActivityKind classify_activity_tile(int tile_index)
    {
        const int last_tile = BOARD_COLUMNS * BOARD_ROWS - 1;
        if (tile_index <= 0 || tile_index >= last_tile)
        {
            return ActivityKind::None;
        }

        if (tile_index % 14 == 0)
        {
            return ActivityKind::Portal;
        }
        if ((tile_index + 5) % 11 == 0)
        {
            return ActivityKind::Slide;
        }
        if ((tile_index % 9) == 0)
        {
            return ActivityKind::Trap;
        }
        if ((tile_index % 4) == 0)
        {
            return ActivityKind::Bonus;
        }

        return ActivityKind::None;
    }

    bool check_wall_collision(const glm::vec3& position, float radius)
    {
        const float half_width = static_cast<float>(BOARD_COLUMNS) * TILE_SIZE * 0.5f;
        const float half_height = static_cast<float>(BOARD_ROWS) * TILE_SIZE * 0.5f;

        if (position.x - radius < -half_width || position.x + radius > half_width)
        {
            return true;
        }
        if (position.z - radius < -half_height || position.z + radius > half_height)
        {
            return true;
        }

        return false;
    }
}

