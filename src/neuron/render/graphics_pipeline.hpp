#pragma once

#include "neuron/base.hpp"
#include "neuron/neuron.hpp"

#include <filesystem>

namespace neuron::render {

    enum class ShaderModuleSourceType { SPIRV, GLSL };

    struct ShaderModulePath {
        std::filesystem::path                  path;
        ShaderModuleSourceType                 type;
        std::optional<vk::ShaderStageFlagBits> stage; // required if ShaderModuleSourceType is glsl
    };

    class NEURON_API ShaderModule {
        ShaderModule();

    public:


    private:
    };

    using ShaderModuleSource = std::variant<vk::ShaderModule, std::shared_ptr<ShaderModule>, ShaderModulePath>;

    struct ShaderStageDefinition {
        ShaderModuleSource      module;
        vk::ShaderStageFlagBits stage;
        std::string             name;
        // TODO: specialization info
    };

    struct NEURON_API GraphicsPipelineBuilder {
        std::vector<ShaderStageDefinition> shader_stages;
        vk::PrimitiveTopology              primitive_topology = vk::PrimitiveTopology::eTriangleList;
    };

    class NEURON_API GraphicsPipeline {
      public:
        GraphicsPipeline(const std::shared_ptr<Context> &context, const GraphicsPipelineBuilder &builder);

        ~GraphicsPipeline();

      private:
    };

} // namespace neuron::render
