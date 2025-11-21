#pragma once

#include <glm/glm.hpp>

namespace game::player::dice
{
    struct DiceState
    {
        glm::vec3 position{};
        glm::vec3 rotation{};
        glm::vec3 rotation_velocity{};
        float roll_duration = 1.0f;
        float roll_timer = 0.0f;
        bool is_rolling = false;
        int result = 0;
        float scale = 1.0f;
    };

    void initialize(DiceState& state, const glm::vec3& position);
    void start_roll(DiceState& state, int result);
    void update(DiceState& state, float delta_time);
    glm::mat4 get_transform(const DiceState& state);
    bool is_roll_complete(const DiceState& state);
}

