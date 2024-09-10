//
// Created by andy on 9/9/24.
//

#include "pipeline_layout.hpp"


namespace neuron::render {
    PipelineLayoutBuilder &PipelineLayoutBuilder::add_push_constant_range(const vk::PushConstantRange &range) {
        push_constant_ranges.push_back(range);
        return *this;
    }

    PipelineLayoutBuilder &PipelineLayoutBuilder::add_push_constant_range(vk::ShaderStageFlags stage_flags, uint32_t offset, uint32_t size) {
        push_constant_ranges.emplace_back(stage_flags, offset, size);
        return *this;
    }

    PipelineLayoutBuilder &PipelineLayoutBuilder::add_descriptor_set_layout(const std::shared_ptr<DescriptorSetLayout> &descriptor_set_layout) {
        descriptor_set_layouts.push_back(descriptor_set_layout);
        return *this;
    }

    std::shared_ptr<PipelineLayout> PipelineLayoutBuilder::build(const std::shared_ptr<Context> &ctx) {
        return std::make_shared<PipelineLayout>(ctx, *this);
    }

    PipelineLayout::PipelineLayout(const std::shared_ptr<Context> &context) : m_context(context) {
        m_pipeline_layout = m_context->device().createPipelineLayout({});
    }

    PipelineLayout::PipelineLayout(const std::shared_ptr<Context> &context, const PipelineLayoutBuilder &builder) : m_context(context) {
        std::vector<vk::DescriptorSetLayout> dsls;
        dsls.reserve(builder.descriptor_set_layouts.size());
        for (const auto& dsl : builder.descriptor_set_layouts) {
            dsls.push_back(dsl->set_layout());
        }
        m_pipeline_layout = m_context->device().createPipelineLayout({{}, dsls, builder.push_constant_ranges});
    }

    PipelineLayout::~PipelineLayout() {
        m_context->device().destroy(m_pipeline_layout);
    }
} // namespace neuron::render
