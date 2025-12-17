#include "camera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace core
{
    Camera::Camera()
        : m_fov(50.0f)
    {
    }

    void Camera::set_fov(float fov)
    {
        m_fov = glm::clamp(fov, MIN_FOV, MAX_FOV);
    }

    float Camera::get_fov() const
    {
        return m_fov;
    }

    void Camera::adjust_fov(float delta)
    {
        m_fov = glm::clamp(m_fov - delta * 5.0f, MIN_FOV, MAX_FOV);
    }

    glm::mat4 Camera::get_projection(float aspect_ratio) const
    {
        return glm::perspective(glm::radians(m_fov), aspect_ratio, 0.1f, 5000.0f);
    }

    glm::mat4 Camera::get_view(const glm::vec3& player_position, float map_length) const
    {
        const float camera_height = std::max(12.0f, map_length * 0.35f);
        const float camera_distance = std::max(18.0f, map_length * 0.55f);
        const glm::vec3 camera_offset(0.0f, camera_height, camera_distance);
        const glm::vec3 camera_position = player_position + camera_offset;
        const glm::vec3 look_at = player_position + glm::vec3(0.0f, 0.0f, -camera_distance * 0.35f);
        return glm::lookAt(camera_position, look_at, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

