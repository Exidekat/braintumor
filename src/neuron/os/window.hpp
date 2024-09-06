//
// Created by andy on 9/6/24.
//

#pragma once

#include "neuron/base.hpp"

#include <memory>
#include <string>

#include "neuron/interface.hpp"

namespace neuron::os {

    struct WindowSettings {
        std::string title;
        int         width, height;
    };

    class Window NEURON_API : public intfc::SurfaceProvider, public intfc::ExtentProvider {
      protected:
        explicit Window(const std::shared_ptr<Context>& context, const WindowSettings &settings);

      public:
        ~Window() override;

        static NEURON_API std::shared_ptr<Window> create(const std::shared_ptr<Context> &context, const WindowSettings &window_settings);

        virtual void post_init();

        static NEURON_API void poll_events();

        [[nodiscard]] bool is_open() const;

        [[nodiscard]] vk::SurfaceKHR get_surface() const override;

        [[nodiscard]] vk::Extent2D get_extent() const override;

      protected:
        GLFWwindow *m_window;

        vk::SurfaceKHR m_surface = nullptr;

        std::shared_ptr<Context> m_context;
    };

} // namespace neuron::os
