//
// Created by andy on 9/6/24.
//

#pragma once

#include "neuron/base.hpp"
#include "neuron/interface.hpp"
#include "neuron/neuron.hpp"

#include <memory>

#include <concepts>

namespace neuron::render {

    struct SwapchainConfiguration {
        vk::Extent2D               extent;
        std::vector<vk::Image>     images;
        std::vector<vk::ImageView> image_views;
    };

    struct DisplayTargetConfiguration {
        vk::Format         format;
        vk::ColorSpaceKHR  color_space;
        uint32_t           min_image_count;
        vk::PresentModeKHR present_mode;
    };

    struct DisplaySystemSettings {
        bool vsync = true;
    };

    struct FrameInfo {
        vk::Semaphore image_available;
        vk::Semaphore render_finished;
        vk::Fence in_flight;

        vk::Image image;
        vk::ImageView image_view;
        uint32_t image_index;

        uint32_t current_frame;
    };

    class DisplaySystem NEURON_API final {
        DisplaySystem(const std::shared_ptr<Context> &context, const DisplaySystemSettings &settings, vk::SurfaceKHR surface);

      public:
        static std::shared_ptr<DisplaySystem> NEURON_API create_raw(const std::shared_ptr<Context> &context, const DisplaySystemSettings &settings, vk::SurfaceKHR surface);

        template <pointer_like<intfc::SurfaceProvider> T>
        static inline std::shared_ptr<DisplaySystem> create(const std::shared_ptr<Context> &context, const DisplaySystemSettings &settings, const T &provider) {
            return create_raw(context, settings, provider->get_surface());
        };

        ~DisplaySystem();

        void set_extent_provider(const std::shared_ptr<intfc::ExtentProvider> &extent_provider);

        [[nodiscard]] vk::SwapchainKHR           swapchain() const;
        [[nodiscard]] SwapchainConfiguration     swapchain_config() const;
        [[nodiscard]] DisplayTargetConfiguration display_target_config() const;
        [[nodiscard]] uint32_t                   current_frame() const;
        [[nodiscard]] uint32_t                   current_image_index() const;

        void build_swapchain();

        [[nodiscard]] const FrameInfo &acquire_next_frame();

        void present_frame();

        static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

      private:
        std::shared_ptr<Context> m_context;

        std::shared_ptr<intfc::ExtentProvider> m_extent_provider;

        vk::SwapchainKHR           m_swapchain;
        SwapchainConfiguration     m_swapchain_config;
        DisplayTargetConfiguration m_display_target_config;

        uint32_t m_current_frame = 0;
        uint32_t m_current_image_index;

        vk::SurfaceKHR m_surface;

        std::vector<vk::Semaphore> m_image_available_semaphores;
        std::vector<vk::Semaphore> m_render_finished_semaphores;
        std::vector<vk::Fence>     m_in_flight_fences;

        FrameInfo m_frame_info;
    };

} // namespace neuron::render
