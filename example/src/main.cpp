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

    auto window = neuron::os::Window::create(ctx, {"Hello!", 800, 600, true});

    auto display_system = neuron::render::DisplaySystem::create(ctx, {.vsync = true}, window);

    auto command_pool    = std::make_shared<neuron::CommandPool>(ctx, ctx->main_queue_family(), true);
    auto command_buffers = command_pool->allocate_command_buffers(neuron::render::DisplaySystem::MAX_FRAMES_IN_FLIGHT);

    auto pipeline_layout = std::make_shared<neuron::render::PipelineLayout>(ctx);

    auto graphics_pipeline_builder = neuron::render::GraphicsPipelineBuilder{.layout = pipeline_layout};
    graphics_pipeline_builder.add_glsl_shader("res/shaders/main.vert")
        .add_glsl_shader("res/shaders/main.frag")
        .add_standard_blend_attachment()
        .add_dynamic_state(vk::DynamicState::eViewport)
        .add_dynamic_state(vk::DynamicState::eScissor)
        .add_viewport({0.0f, 0.0f}, we, 0.0f, 1.0f);

    vk::Extent2D we = window->get_extent();

    graphics_pipeline_builder.viewports.emplace_back(0.0f, 0.0f, static_cast<float>(we.width), static_cast<float>(we.height), 0.0f, 1.0f);
    graphics_pipeline_builder.scissors.emplace_back(vk::Offset2D{0, 0}, we);


    graphics_pipeline_builder.color_attachment_formats = {display_system->display_target_config().format};

    std::shared_ptr<neuron::render::GraphicsPipeline> graphics_pipeline = std::make_shared<neuron::render::GraphicsPipeline>(ctx, graphics_pipeline_builder);

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

    ctx->device().destroy(command_pool);

    std::cout << "Best FPS: " << best_fps << std::endl;
}
