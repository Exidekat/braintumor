#pragma once

#include "neuron/base.hpp"
#include "neuron/neuron.hpp"
#include "pipeline_layout.hpp"

#include <filesystem>

#include <glm/glm.hpp>

namespace neuron::render {

    enum class ShaderModuleSourceType { SPIRV, GLSL };

    struct ShaderCode {
        std::variant<std::string, std::vector<uint32_t>> code;
    };

    using ShaderModuleCodeSource = std::variant<std::filesystem::path, ShaderCode>;

    struct ShaderModuleInfo {
        ShaderModuleCodeSource  source;
        ShaderModuleSourceType  type;
        vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eAll; // eAll means infer from source
    };

    class NEURON_API ShaderModule {
        ShaderModule(const std::shared_ptr<Context> &context, const ShaderModuleInfo &info);

      public:
        static std::shared_ptr<ShaderModule> load(const std::shared_ptr<Context> &context, const ShaderModuleInfo &info);

        ~ShaderModule();

        ShaderModule(const ShaderModule &other)     = delete;
        ShaderModule(ShaderModule &&other) noexcept = default;

        ShaderModule &operator=(const ShaderModule &other)     = delete;
        ShaderModule &operator=(ShaderModule &&other) noexcept = default;

        [[nodiscard]] inline vk::ShaderModule module() const { return m_module; }

      private:
        std::shared_ptr<Context> m_context;
        vk::ShaderModule         m_module;
    };

    using ShaderModuleSource = std::variant<vk::ShaderModule, std::shared_ptr<ShaderModule>, ShaderModuleInfo>;

    struct ShaderStageDefinition {
        ShaderModuleSource      module;
        vk::ShaderStageFlagBits stage;
        // TODO: specialization info
    };

    class GraphicsPipeline;

    struct NEURON_API GraphicsPipelineBuilder {
        std::vector<ShaderStageDefinition>               shader_stages;
        std::unordered_set<vk::DynamicState>             dynamic_states;
        std::vector<vk::VertexInputBindingDescription>   vertex_bindings;
        std::vector<vk::VertexInputAttributeDescription> vertex_attributes;

        std::vector<vk::Viewport> viewports;
        std::vector<vk::Rect2D>   scissors;

        vk::PrimitiveTopology primitive_topology       = vk::PrimitiveTopology::eTriangleList;
        bool                  enable_primitive_restart = false;

        uint32_t patch_control_points = 1;

        bool              enable_depth_clamp         = false;
        bool              enable_rasterizer_discard  = false;
        vk::PolygonMode   polygon_mode               = vk::PolygonMode::eFill;
        vk::CullModeFlags cull_mode                  = vk::CullModeFlagBits::eBack;
        vk::FrontFace     front_face                 = vk::FrontFace::eClockwise;
        bool              enable_depth_bias          = false;
        float             depth_bias_constant_factor = 0.0f;
        float             depth_bias_clamp           = 0.0f;
        float             depth_bias_slope_factor    = 0.0f;
        float             line_width                 = 1.0f;


        vk::SampleCountFlagBits     rasterization_samples = vk::SampleCountFlagBits::e1;
        bool                        enable_sample_shading = false;
        float                       min_sample_shading    = 1.0f;
        std::vector<vk::SampleMask> sample_mask;
        bool                        enable_alpha_to_coverage = false;
        bool                        enable_alpha_to_one      = false;

        bool               enable_depth_test        = false;
        bool               enable_depth_write       = false;
        vk::CompareOp      depth_compare_op         = vk::CompareOp::eLess;
        bool               enable_depth_bounds_test = false;
        bool               enable_stencil_test      = false;
        vk::StencilOpState stencil_front            = {vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways, ~0U, 0U, 0U};
        vk::StencilOpState stencil_back             = {vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways, ~0U, 0U, 0U};
        float              min_depth_bounds         = 0.0f;
        float              max_depth_bounds         = 1.0f;

        bool                                               enable_logic_op = false;
        vk::LogicOp                                        logic_op        = vk::LogicOp::eClear;
        std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
        std::array<float, 4>                               blend_constants = {0.0f, 0.0f, 0.0f, 0.0f};

        std::shared_ptr<PipelineLayout> layout;
        vk::RenderPass                  render_pass = nullptr;
        uint32_t                        subpass     = 0;

        vk::Pipeline base_pipeline       = VK_NULL_HANDLE;
        int32_t      base_pipeline_index = -1;

        std::vector<vk::Format> color_attachment_formats;
        vk::Format              depth_format;
        vk::Format              stencil_format;


        GraphicsPipelineBuilder &add_shader(const ShaderStageDefinition &def);
        GraphicsPipelineBuilder &add_shader(vk::ShaderStageFlagBits stage, const ShaderModuleInfo &module);
        GraphicsPipelineBuilder &add_shader(ShaderModuleSourceType source_type, vk::ShaderStageFlagBits stage, const ShaderModuleCodeSource &source);
        GraphicsPipelineBuilder &add_glsl_shader(const std::filesystem::path &path);
        GraphicsPipelineBuilder &add_blend_attachment(const vk::PipelineColorBlendAttachmentState &blend_attachment);
        GraphicsPipelineBuilder &add_dynamic_state(vk::DynamicState state);
        GraphicsPipelineBuilder &add_vertex_binding(uint32_t binding, uint32_t stride, vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex);
        GraphicsPipelineBuilder &add_vertex_attribute(uint32_t binding, uint32_t location, vk::Format format, uint32_t offset);
        GraphicsPipelineBuilder &add_standard_blend_attachment();
        GraphicsPipelineBuilder &add_viewport(const glm::fvec2& pos, const vk::Extent2D& extent, float min_depth, float max_depth);
        GraphicsPipelineBuilder &add_viewport(const glm::fvec2& pos, const glm::fvec2& extent, float min_depth, float max_depth);
        GraphicsPipelineBuilder &add_scissor(const vk::Rect2D& scissor);
        GraphicsPipelineBuilder &add_scissor(const vk::Offset2D& offset, const vk::Extent2D& extent);
        GraphicsPipelineBuilder &add_color_attachment_with_standard_blend(vk::Format format);
        GraphicsPipelineBuilder &set_color_attachment_format(size_t index, vk::Format format);
        GraphicsPipelineBuilder &set_depth_attachment_format(vk::Format format);
        GraphicsPipelineBuilder &set_stencil_attachment_format(vk::Format format);
        GraphicsPipelineBuilder &set_blend_attachment(size_t index, const vk::PipelineColorBlendAttachmentState& blend_attachment);
        GraphicsPipelineBuilder &set_standard_blend_attachment(size_t index);

        std::shared_ptr<GraphicsPipeline> build(const std::shared_ptr<Context> &ctx);

        explicit inline GraphicsPipelineBuilder(const std::shared_ptr<PipelineLayout> &layout_) : layout(layout_) {}
    };

    class NEURON_API GraphicsPipeline {
      public:
        GraphicsPipeline(const std::shared_ptr<Context> &context, const GraphicsPipelineBuilder &builder);

        ~GraphicsPipeline();

        [[nodiscard]] inline vk::Pipeline pipeline() const { return m_pipeline; }

      private:
        std::shared_ptr<Context>                   m_context;
        std::vector<std::shared_ptr<ShaderModule>> m_shader_modules;


        vk::Pipeline m_pipeline;
    };

} // namespace neuron::render
