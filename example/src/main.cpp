#include "neuron/neuron.hpp"
#include "neuron/os/window.hpp"
#include "neuron/render/display_system.hpp"
#include "neuron/render/graphics_pipeline.hpp"
#include "neuron/render/simple_render_pass.hpp"


#include <iostream>
#include <memory>

int main() {
    std::cout << "Running Neuron version: " << neuron::get_version() << std::endl;

    auto ctx = std::make_shared<neuron::Context>(neuron::ContextSettings{
        .application_name = "neuron-example", .application_version = neuron::Version{0, 1, 0}, .enable_api_validation = true,
        // .enable_api_dump       = true,
    });

    auto         window          = neuron::os::Window::create(ctx, {"Hello!", 800, 600, true});
    vk::Extent2D original_extent = window->get_extent();


    auto display_system = neuron::render::DisplaySystem::create(ctx, {.vsync = true}, window);

    auto command_pool    = std::make_shared<neuron::CommandPool>(ctx, ctx->main_queue_family(), true);
    auto command_buffers = command_pool->allocate_command_buffers(neuron::render::DisplaySystem::MAX_FRAMES_IN_FLIGHT);

    // auto pipline_layout_builder

    auto pipeline_layout = neuron::render::PipelineLayoutBuilder().add_push_constant_range(vk::ShaderStageFlagBits::eVertex, 0, 4).build(ctx);

    auto graphics_pipeline = neuron::render::GraphicsPipelineBuilder(pipeline_layout)
                                 .add_glsl_shader("res/shaders/main.vert")
                                 .add_glsl_shader("res/shaders/main.frag")
                                 .add_viewport({0.0f, 0.0f}, original_extent, 0.0f, 1.0f)
                                 .add_scissor({0, 0}, original_extent)
                                 .add_dynamic_state(vk::DynamicState::eViewport)
                                 .add_dynamic_state(vk::DynamicState::eScissor)
                                 .add_color_attachment_with_standard_blend(display_system->display_target_config().format)
                                 .set_depth_attachment_format(vk::Format::eD24UnormS8Uint)
                                 .set_stencil_attachment_format(vk::Format::eD24UnormS8Uint)
                                 .build(ctx);

    double last_frame = -std::numeric_limits<double>::infinity();
    double this_frame = glfwGetTime();

    double best_fps = 0.0f;

    while (window->is_open()) {
        neuron::os::Window::poll_events();

        auto frame_info = display_system->acquire_next_frame();

        auto render_area = vk::Rect2D{{0, 0}, display_system->swapchain_config().extent};

        vk::CommandBuffer cmd = command_buffers[frame_info.current_frame];
        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        neuron::render::SimpleRenderPassInfo pass_info{frame_info.image, frame_info.image_view, render_area, {0.0f, 0.0f, 0.0f, 1.0f}, true};

        neuron::render::simple_render_pass(cmd, pass_info, [&](const vk::CommandBuffer &cmd) {
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline->pipeline());
            cmd.setViewport(0, vk::Viewport{0.0f, 0.0f, static_cast<float>(render_area.extent.width), static_cast<float>(render_area.extent.height), 0.0f, 1.0f});

            vk::Rect2D s = {{0, 0}, {render_area.extent.width / 2, render_area.extent.height}};
            cmd.setScissor(0, s);

            const auto time = static_cast<float>(glfwGetTime());

            cmd.pushConstants(pipeline_layout->pipeline_layout(), vk::ShaderStageFlagBits::eVertex, 0, 4, &time);

            cmd.draw(3, 1, 0, 0);


            s.offset.x = static_cast<int>(render_area.extent.width) / 2;
            cmd.setScissor(0, s);

            cmd.draw(3, 1, 0, 1);
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

        last_frame = this_frame;
        this_frame = glfwGetTime();
        double fps = 1.0 / (this_frame - last_frame);
        best_fps   = std::max(fps, best_fps);
    }


    ctx->device().waitIdle();

    std::cout << "Best FPS: " << best_fps << std::endl;
}
