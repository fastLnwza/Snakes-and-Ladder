#pragma once

#include "core/types.h"
#include <vector>

namespace game::map
{
    std::pair<std::vector<Vertex>, std::vector<unsigned int>> build_snakes_ladders_map();
}

