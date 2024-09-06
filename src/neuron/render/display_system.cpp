//
// Created by andy on 9/6/24.
//

#include "display_system.hpp"

#include <iostream>

namespace neuron::render {
    vk::PresentModeKHR select_present_mode(const std::vector<vk::PresentModeKHR> &present_modes, bool vsync) {
        bool immediate    = false;
        bool fifo_relaxed = false;

        for (const auto &present_mode : present_modes) {
            if (present_mode == vk::PresentModeKHR::eMailbox) {
                return present_mode;
            }

            if (present_mode == vk::PresentModeKHR::eFifoRelaxed) {
                fifo_relaxed = true;
            }

            if (present_mode == vk::PresentModeKHR::eImmediate) {
                immediate = true;
            }
        }

        if (vsync) {
            return vk::PresentModeKHR::eFifo;
        }

        if (immediate) {
            return vk::PresentModeKHR::eImmediate;
        }

        if (fifo_relaxed) {
            return vk::PresentModeKHR::eFifoRelaxed;
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::SurfaceFormatKHR select_surface_format(const std::vector<vk::SurfaceFormatKHR> &formats) {
        bool rgba_unorm = false;
        bool bgra_unorm = false;
        bool rgba_srgb  = false;

        for (const auto &sf : formats) {
            if (sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                if (sf.format == vk::Format::eB8G8R8A8Srgb) {
                    return sf;
                }
                if (sf.format == vk::Format::eR8G8B8A8Srgb) {
                    rgba_srgb = true;
                }
                if (sf.format == vk::Format::eB8G8R8A8Unorm) {
                    bgra_unorm = true;
                }
                if (sf.format == vk::Format::eR8G8B8A8Unorm) {
                    rgba_unorm = true;
                }
            }
        }

        if (rgba_srgb)
            return {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};
        if (bgra_unorm)
            return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
        if (rgba_unorm)
            return {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
        return formats.front();
    }

    DisplaySystem::DisplaySystem(const std::shared_ptr<Context> &context, const DisplaySystemSettings &settings, vk::SurfaceKHR surface) : m_context{context}, m_surface{surface} {
        vk::SurfaceCapabilitiesKHR caps          = m_context->physical_device().getSurfaceCapabilitiesKHR(m_surface);
        auto                       present_modes = m_context->physical_device().getSurfacePresentModesKHR(m_surface);
        auto                       formats       = m_context->physical_device().getSurfaceFormatsKHR(m_surface);

        m_display_target_config.present_mode = select_present_mode(present_modes, settings.vsync);

        auto surface_format                 = select_surface_format(formats);
        m_display_target_config.format      = surface_format.format;
        m_display_target_config.color_space = surface_format.colorSpace;

        m_display_target_config.min_image_count = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && caps.maxImageCount < m_display_target_config.min_image_count) {
            m_display_target_config.min_image_count = caps.maxImageCount;
        }

        build_swapchain();

        m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
        m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_in_flight_fences[i]           = m_context->device().createFence(vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
            m_image_available_semaphores[i] = m_context->device().createSemaphore(vk::SemaphoreCreateInfo{});
            m_render_finished_semaphores[i] = m_context->device().createSemaphore(vk::SemaphoreCreateInfo{});
        }
    }

    std::shared_ptr<DisplaySystem> DisplaySystem::create_raw(const std::shared_ptr<Context> &context, const DisplaySystemSettings &settings, vk::SurfaceKHR surface) {
        return std::shared_ptr<DisplaySystem>(new DisplaySystem(context, settings, surface));
    }

    DisplaySystem::~DisplaySystem() {
        for (const auto &iv : m_swapchain_config.image_views) {
            m_context->device().destroy(iv);
        }

        if (m_swapchain) {
            m_context->device().destroy(m_swapchain);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_context->device().destroy(m_image_available_semaphores[i]);
            m_context->device().destroy(m_render_finished_semaphores[i]);
            m_context->device().destroy(m_in_flight_fences[i]);
        }
    }

    void DisplaySystem::set_extent_provider(const std::shared_ptr<intfc::ExtentProvider> &extent_provider) {
        m_extent_provider = extent_provider;
    }

    vk::SwapchainKHR DisplaySystem::swapchain() const {
        return m_swapchain;
    }

    SwapchainConfiguration DisplaySystem::swapchain_config() const {
        return m_swapchain_config;
    }

    DisplayTargetConfiguration DisplaySystem::display_target_config() const {
        return m_display_target_config;
    }

    uint32_t DisplaySystem::current_frame() const {
        return m_current_frame;
    }

    uint32_t DisplaySystem::current_image_index() const {
        return m_current_image_index;
    }

    void DisplaySystem::build_swapchain() {
        vk::SurfaceCapabilitiesKHR caps = m_context->physical_device().getSurfaceCapabilitiesKHR(m_surface);

        SwapchainConfiguration old_configuration = m_swapchain_config;

        if (caps.currentExtent.height == std::numeric_limits<uint32_t>::max()) {
            if (m_extent_provider) {
                auto extent = m_extent_provider->get_extent();

                m_swapchain_config.extent = vk::Extent2D{
                    std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width),
                    std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height),
                };
            } else {
                throw std::runtime_error("No extent provider set & surface capabilities cannot provide extent.");
            }
        } else {
            m_swapchain_config.extent = caps.currentExtent;
        }


        vk::SwapchainCreateInfoKHR create_info{};

        create_info.surface          = m_surface;
        create_info.minImageCount    = m_display_target_config.min_image_count;
        create_info.imageFormat      = m_display_target_config.format;
        create_info.imageColorSpace  = m_display_target_config.color_space;
        create_info.imageExtent      = m_swapchain_config.extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst;
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.preTransform     = caps.currentTransform;
        create_info.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        create_info.presentMode      = m_display_target_config.present_mode;
        create_info.clipped          = true;
        create_info.oldSwapchain     = m_swapchain;

        m_swapchain = m_context->device().createSwapchainKHR(create_info);

        if (create_info.oldSwapchain) {
            for (const auto &iv : m_swapchain_config.image_views) {
                m_context->device().destroy(iv);
            }

            m_context->device().destroy(create_info.oldSwapchain);
        }

        m_swapchain_config.images = m_context->device().getSwapchainImagesKHR(m_swapchain);
        m_swapchain_config.image_views.clear();
        m_swapchain_config.image_views.reserve(m_swapchain_config.images.size());

        for (const auto &image : m_swapchain_config.images) {
            m_swapchain_config.image_views.push_back(m_context->device().createImageView(
                vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, m_display_target_config.format,
                                        vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA),
                                        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))));
        }
    }

    const FrameInfo &DisplaySystem::acquire_next_frame() {
        auto _ = m_context->device().waitForFences(m_in_flight_fences[m_current_frame], true, UINT64_MAX);
        m_context->device().resetFences(m_in_flight_fences[m_current_frame]);

        bool acquired = false;

        do {
            try {
                auto res = m_context->device().acquireNextImageKHR(m_swapchain, UINT64_MAX, m_image_available_semaphores[m_current_frame], VK_NULL_HANDLE);
                if (res.result == vk::Result::eErrorOutOfDateKHR || res.result == vk::Result::eSuboptimalKHR) {
                    m_context->device().waitIdle();
                    build_swapchain();
                    continue;
                }

                m_frame_info.image_index = res.value;

                acquired = true;
            } catch (vk::OutOfDateKHRError &e) {
                m_context->device().waitIdle();
                build_swapchain();
            }
        } while (!acquired);

        m_frame_info.image      = m_swapchain_config.images[m_frame_info.image_index];
        m_frame_info.image_view = m_swapchain_config.image_views[m_frame_info.image_index];

        m_frame_info.image_available = m_image_available_semaphores[m_current_frame];
        m_frame_info.render_finished = m_render_finished_semaphores[m_current_frame];
        m_frame_info.in_flight       = m_in_flight_fences[m_current_frame];

        m_frame_info.current_frame = m_current_frame;

        return m_frame_info;
    }

    void DisplaySystem::present_frame() {
        vk::PresentInfoKHR present_info{};
        present_info.setSwapchains(m_swapchain);
        present_info.setImageIndices(m_frame_info.image_index);
        present_info.setWaitSemaphores(m_frame_info.render_finished);

        try {
            vk::Result result = m_context->main_queue().presentKHR(present_info);
            if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
                m_context->device().waitIdle();
                build_swapchain();
            }
        } catch (vk::OutOfDateKHRError &e) {
            m_context->device().waitIdle();
            build_swapchain();
        }

        m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
} // namespace neuron::render
