#include "neuron/neuron.hpp"
#include "neuron/os/window.hpp"
#include "neuron/render/display_system.hpp"


#include <iostream>
#include <memory>

int main() {
    std::cout << "Running Neuron version: " << neuron::get_version() << std::endl;

    auto ctx = std::make_shared<neuron::Context>(neuron::ContextSettings{
        .application_name = "neuron-example", .application_version = neuron::Version{0, 1, 0}, .enable_api_validation = true,
        // .enable_api_dump       = true,
    });

    auto window = neuron::os::Window::create(ctx, {"Hello!", 800, 600});

    auto display_system = neuron::render::DisplaySystem::create(ctx, {.vsync = true}, window);

    vk::CommandPool command_pool = ctx->device().createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, ctx->main_queue_family()));
    std::vector<vk::CommandBuffer> command_buffers = ctx->device().allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(command_pool, vk::CommandBufferLevel::ePrimary, neuron::render::DisplaySystem::MAX_FRAMES_IN_FLIGHT));

    while (window->is_open()) {
        neuron::os::Window::poll_events();

        auto frame_info = display_system->acquire_next_frame();

        vk::CommandBuffer cmd = command_buffers[frame_info.current_frame];
        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        // ilt1
        {
            vk::ImageMemoryBarrier b{};
            b.image            = frame_info.image;
            b.srcAccessMask    = vk::AccessFlagBits::eNone;
            b.dstAccessMask    = vk::AccessFlagBits::eColorAttachmentWrite;
            b.oldLayout        = vk::ImageLayout::eUndefined;
            b.newLayout        = vk::ImageLayout::eColorAttachmentOptimal;
            b.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, b);
        }

        vk::ClearColorValue clear_value{1.0f, 0.0f, 0.0f, 1.0f};

        vk::RenderingAttachmentInfo attachment{};
        attachment.setClearValue(clear_value);
        attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
        attachment.setImageView(frame_info.image_view);
        attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        attachment.setStoreOp(vk::AttachmentStoreOp::eStore);

        cmd.beginRendering(vk::RenderingInfo{
            {}, vk::Rect2D{{0, 0}, window->get_extent()}, 1, 0, attachment
        });

        cmd.endRendering();

        // ilt2
        {
            vk::ImageMemoryBarrier b{};
            b.image            = frame_info.image;
            b.srcAccessMask    = vk::AccessFlagBits::eColorAttachmentWrite;
            b.dstAccessMask    = vk::AccessFlagBits::eNone;
            b.oldLayout        = vk::ImageLayout::eColorAttachmentOptimal;
            b.newLayout        = vk::ImageLayout::ePresentSrcKHR;
            b.subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, b);
        }

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
