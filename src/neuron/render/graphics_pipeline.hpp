#pragma once

#include "neuron/base.hpp"
#include "neuron/neuron.hpp"

#include <filesystem>

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

        [[nodiscard]] vk::ShaderModule module() const { return m_module; }

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

    struct NEURON_API GraphicsPipelineBuilder {
        std::vector<ShaderStageDefinition>               shader_stages;
        std::vector<vk::DynamicState>                    dynamic_states;
        std::vector<vk::VertexInputBindingDescription>   vertex_bindings;
        std::vector<vk::VertexInputAttributeDescription> vertex_attributes;

        std::vector<vk::Viewport> viewports;
        std::vector<vk::Rect2D> scissors;

        vk::PrimitiveTopology primitive_topology = vk::PrimitiveTopology::eTriangleList;
        bool enable_primitive_restart = false;

        bool enable_depth_clamp = false;
        vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
        float line_width = 1.0f;
        vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;
        vk::FrontFace front_face = vk::FrontFace::eClockwise;

        bool enable_depth_bias = false;
        float depth_bias_constant_factor = 0.0f;
        float depth_bias_clamp = 0.0f;
        float depth_bias_clamp = 0.0f;
    };

    class NEURON_API GraphicsPipeline {
      public:
        GraphicsPipeline(const std::shared_ptr<Context> &context, const GraphicsPipelineBuilder &builder);

        ~GraphicsPipeline();

      private:
        std::shared_ptr<Context>                   m_context;
        std::vector<std::shared_ptr<ShaderModule>> m_shader_modules;
    };

} // namespace neuron::render
