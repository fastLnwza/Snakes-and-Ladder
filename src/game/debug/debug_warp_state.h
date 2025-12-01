#pragma once

#include <array>
#include <string>

struct DebugWarpState
{
    bool active = false;
    std::string buffer;
    std::array<bool, 10> digit_previous{};
    bool prev_toggle = false;
    bool prev_enter = false;
    bool prev_backspace = false;
    float notification_timer = 0.0f;
    std::string notification;
};



