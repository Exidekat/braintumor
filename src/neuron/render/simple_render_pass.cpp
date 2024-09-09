#include "simple_render_pass.hpp"

namespace neuron::render {

    void start_simple_render_pass(const vk::CommandBuffer &cmd, const SimpleRenderPassInfo &info) {
        {
            vk::ImageMemoryBarrier b{};
            b.image            = info.image;
            b.srcAccessMask    = vk::AccessFlagBits::eNone;
            b.dstAccessMask    = info.target_access;
            b.oldLayout        = vk::ImageLayout::eUndefined;
            b.newLayout        = info.target_layout;
            b.subresourceRange = info.isr;

            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, info.target_stage, {}, {}, {}, b);
        }

        vk::RenderingAttachmentInfo attachment{};
        attachment.setClearValue(info.clear_value);
        attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
        attachment.setImageView(info.image_view);
        attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        attachment.setStoreOp(vk::AttachmentStoreOp::eStore);

        cmd.beginRendering(vk::RenderingInfo{{}, info.render_area, 1, 0, attachment});
    }

    void end_simple_render_pass(const vk::CommandBuffer &cmd, const SimpleRenderPassInfo &info) {
        cmd.endRendering();

        if (info.present_compatible) {
            vk::ImageMemoryBarrier b{};
            b.image            = info.image;
            b.srcAccessMask    = info.target_access;
            b.dstAccessMask    = vk::AccessFlagBits::eNone;
            b.oldLayout        = info.target_layout;
            b.newLayout        = vk::ImageLayout::ePresentSrcKHR;
            b.subresourceRange = info.isr;

            cmd.pipelineBarrier(info.target_stage, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, b);
        }
    }

    void simple_render_pass(const vk::CommandBuffer &commandBuffer, const SimpleRenderPassInfo &info, const std::function<void(const vk::CommandBuffer &)> &f) {
        start_simple_render_pass(commandBuffer, info);
        f(commandBuffer);
        end_simple_render_pass(commandBuffer, info);
    }
} // namespace neuron::render
