//
// Created by andy on 9/6/24.
//

#include "window.hpp"

#include "neuron/neuron.hpp"

namespace neuron::os {

    Window::Window(const std::shared_ptr<Context> &context, const WindowSettings &settings) : m_context(context) {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(settings.width, settings.height, settings.title.c_str(), nullptr, nullptr);

        VkSurfaceKHR s;
        glfwCreateWindowSurface(m_context->instance(), m_window, nullptr, &s);
        m_surface = s;
    }

    Window::~Window() {
        if (m_surface) {
            m_context->instance().destroy(m_surface);
        }

        glfwDestroyWindow(m_window);
    }

    std::shared_ptr<Window> Window::create(const std::shared_ptr<Context> &context, const WindowSettings &window_settings) {
        auto window = std::shared_ptr<Window>(new Window(context, window_settings));
        window->post_init();

        return window;
    }

    void Window::post_init() {}

    void Window::poll_events() {
        glfwPollEvents();
    }

    bool Window::is_open() const {
        return !glfwWindowShouldClose(m_window);
    }

    vk::SurfaceKHR Window::get_surface() const {
        return m_surface;
    }

    vk::Extent2D Window::get_extent() const {
        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);
        return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    }
} // namespace neuron::os
