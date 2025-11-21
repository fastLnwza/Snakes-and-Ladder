#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace core
{
    class Camera
    {
    public:
        Camera();
        
        void set_fov(float fov);
        float get_fov() const;
        void adjust_fov(float delta);
        
        glm::mat4 get_projection(float aspect_ratio) const;
        glm::mat4 get_view(const glm::vec3& player_position, float map_length) const;

    private:
        float m_fov = 50.0f;
        static constexpr float MIN_FOV = 30.0f;
        static constexpr float MAX_FOV = 85.0f;
    };
}

