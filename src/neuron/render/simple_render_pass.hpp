#pragma once

#include "neuron/base.hpp"
#include "neuron/neuron.hpp"

namespace neuron::render {

    struct SimpleRenderPassInfo {
        vk::Image           image;
        vk::ImageView       image_view;
        vk::Rect2D          render_area;
        vk::ClearColorValue clear_value;

        bool present_compatible;

        vk::ImageLayout           target_layout = vk::ImageLayout::eColorAttachmentOptimal;
        vk::AccessFlags           target_access = vk::AccessFlagBits::eColorAttachmentWrite;
        vk::PipelineStageFlags    target_stage  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::ImageSubresourceRange isr           = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    };

    void NEURON_API start_simple_render_pass(const vk::CommandBuffer &commandBuffer, const SimpleRenderPassInfo &info);
    void NEURON_API end_simple_render_pass(const vk::CommandBuffer &commandBuffer, const SimpleRenderPassInfo &info);

    void NEURON_API simple_render_pass(const vk::CommandBuffer &commandBuffer, const SimpleRenderPassInfo &info, const std::function<void(const vk::CommandBuffer&)> &f);

} // namespace neuron::render
