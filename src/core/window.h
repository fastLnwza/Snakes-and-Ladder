#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <functional>

namespace core
{
    class Window
    {
    public:
        Window(int width, int height, const char* title);
        ~Window();
        
        bool should_close() const;
        void poll_events();
        void swap_buffers();
        
        GLFWwindow* get_handle() { return m_window; }
        
        bool is_key_pressed(int key) const;
        void close();
        
        void get_framebuffer_size(int* width, int* height) const;
        float get_aspect_ratio() const;
        
        void set_scroll_callback(std::function<void(double, double)> callback);

    private:
        GLFWwindow* m_window = nullptr;
    };
}

