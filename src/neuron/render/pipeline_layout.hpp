//
// Created by andy on 9/9/24.
//

#pragma once

#include "neuron/base.hpp"
#include "neuron/neuron.hpp"

#include <memory>

namespace neuron::render {

    class NEURON_API DescriptorSetLayout {
    public:


        [[nodiscard]] vk::DescriptorSetLayout set_layout() const { return m_set_layout; }
    private:
        std::shared_ptr<Context> m_context;
        vk::DescriptorSetLayout m_set_layout;
    };

    class PipelineLayout;

    struct NEURON_API PipelineLayoutBuilder {
        std::vector<std::shared_ptr<DescriptorSetLayout>> descriptor_set_layouts;
        std::vector<vk::PushConstantRange> push_constant_ranges;


        PipelineLayoutBuilder &add_push_constant_range(const vk::PushConstantRange& range);
        PipelineLayoutBuilder &add_push_constant_range(vk::ShaderStageFlags stage_flags, uint32_t offset, uint32_t size);

        PipelineLayoutBuilder &add_descriptor_set_layout(const std::shared_ptr<DescriptorSetLayout>& descriptor_set_layout);

        std::shared_ptr<PipelineLayout> build(const std::shared_ptr<Context>& ctx);

    };

    class NEURON_API PipelineLayout {
      public:
        explicit PipelineLayout(const std::shared_ptr<Context> &context);
        PipelineLayout(const std::shared_ptr<Context> &context, const PipelineLayoutBuilder& builder);

        ~PipelineLayout();

        [[nodiscard]] vk::PipelineLayout pipeline_layout() const { return m_pipeline_layout; }

      private:

        std::shared_ptr<Context> m_context;
        vk::PipelineLayout m_pipeline_layout;
    };

} // namespace neuron::render
