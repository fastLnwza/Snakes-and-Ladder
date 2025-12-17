#include "window.h"

#include <iostream>
#include <stdexcept>

namespace core
{
    namespace
    {
        Window* g_window_instance = nullptr;
        std::function<void(double, double)> g_scroll_callback;

        void framebuffer_size_callback(GLFWwindow* window, int width, int height)
        {
            (void)window;
            glViewport(0, 0, width, height);
        }

        void scroll_callback_wrapper(GLFWwindow* window, double x_offset, double y_offset)
        {
            (void)window;
            if (g_scroll_callback)
            {
                g_scroll_callback(x_offset, y_offset);
            }
        }
    }

    Window::Window(int width, int height, const char* title)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_window)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(m_window);
        glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
        glfwSetScrollCallback(m_window, scroll_callback_wrapper);
        glfwSwapInterval(1);

        if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
        {
            glfwDestroyWindow(m_window);
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD");
        }

        g_window_instance = this;
    }

    Window::~Window()
    {
        if (m_window)
        {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
        g_window_instance = nullptr;
    }

    bool Window::should_close() const
    {
        return glfwWindowShouldClose(m_window);
    }

    void Window::poll_events()
    {
        glfwPollEvents();
    }

    void Window::swap_buffers()
    {
        glfwSwapBuffers(m_window);
    }

    bool Window::is_key_pressed(int key) const
    {
        return glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    void Window::close()
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    void Window::get_framebuffer_size(int* width, int* height) const
    {
        glfwGetFramebufferSize(m_window, width, height);
    }

    float Window::get_aspect_ratio() const
    {
        int width = 0;
        int height = 0;
        get_framebuffer_size(&width, &height);
        return static_cast<float>(width > 0 ? width : 1) / static_cast<float>(height > 0 ? height : 1);
    }

    void Window::set_scroll_callback(std::function<void(double, double)> callback)
    {
        g_scroll_callback = callback;
    }
}

