#include "neuron/neuron.hpp"
#include "neuron/os/window.hpp"
#include "neuron/render/display_system.hpp"
#include "neuron/render/simple_render_pass.hpp"


#include <iostream>
#include <memory>

int main() {
    std::cout << "Running Neuron version: " << neuron::get_version() << std::endl;

    auto ctx = std::make_shared<neuron::Context>(neuron::ContextSettings{
        .application_name = "neuron-example", .application_version = neuron::Version{0, 1, 0}, .enable_api_validation = true,
        // .enable_api_dump       = true,
    });

    auto window = neuron::os::Window::create(ctx, {"Hello!", 800, 600, true});

    auto display_system = neuron::render::DisplaySystem::create(ctx, {.vsync = true}, window);

    vk::CommandPool command_pool = ctx->device().createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, ctx->main_queue_family()));
    std::vector<vk::CommandBuffer> command_buffers = ctx->device().allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(command_pool, vk::CommandBufferLevel::ePrimary, neuron::render::DisplaySystem::MAX_FRAMES_IN_FLIGHT));

    while (window->is_open()) {
        neuron::os::Window::poll_events();

        auto frame_info = display_system->acquire_next_frame();

        auto render_area = vk::Rect2D{{0, 0}, display_system->swapchain_config().extent};

        vk::CommandBuffer cmd = command_buffers[frame_info.current_frame];
        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        neuron::render::SimpleRenderPassInfo pass_info{frame_info.image, frame_info.image_view, render_area, {0.0f, 1.0f, 0.0f, 1.0f}, true};

        neuron::render::simple_render_pass(cmd, pass_info, [&](const vk::CommandBuffer &cmd) {

        });

        cmd.end();

        vk::SubmitInfo si{};
        si.setCommandBuffers(cmd);
        si.setWaitSemaphores(frame_info.image_available);
        si.setSignalSemaphores(frame_info.render_finished);

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eTopOfPipe;
        si.setWaitDstStageMask(waitStage);

        ctx->main_queue().submit(si, frame_info.in_flight);

        // TODO: rendering. current setup will fail to run

        display_system->present_frame();
    }

    ctx->device().waitIdle();

    ctx->device().destroy(command_pool);
}
