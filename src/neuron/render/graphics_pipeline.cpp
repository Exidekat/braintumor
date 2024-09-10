#include "graphics_pipeline.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <shaderc/shaderc.hpp>

namespace neuron::render {
    static std::string read_file_text(const std::filesystem::path &path) {
        std::ifstream file(path.string(), std::ios::ate | std::ios::in);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file");
        }

        auto  file_size = static_cast<std::streamsize>(file.tellg());
        auto *buffer    = new char[file_size + 1];
        memset(buffer, 0, file_size + 1);
        file.seekg(0);
        file.read(buffer, file_size);
        file.close();
        std::string content(buffer, strlen(buffer));
        delete[] buffer;
        return content;
    }

    static std::vector<uint32_t> read_file_binary(const std::filesystem::path &path) {
        std::ifstream file(path.string(), std::ios::ate | std::ios::in | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file");
        }

        auto                  file_size   = file.tellg();
        size_t                buffer_size = file_size / sizeof(uint32_t);
        std::vector<uint32_t> buffer(buffer_size);
        file.seekg(0);
        file.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(buffer_size * sizeof(uint32_t)));
        file.close();
        return buffer;
    }

    std::vector<uint32_t> compile_glsl(std::string glsl, vk::ShaderStageFlagBits stage) {
        shaderc_shader_kind kind = shaderc_glsl_infer_from_source;

        switch (stage) {
        case vk::ShaderStageFlagBits::eVertex:
            kind = shaderc_glsl_vertex_shader;
            break;
        case vk::ShaderStageFlagBits::eFragment:
            kind = shaderc_glsl_fragment_shader;
            break;
        case vk::ShaderStageFlagBits::eTessellationControl:
            kind = shaderc_glsl_tess_control_shader;
            break;
        case vk::ShaderStageFlagBits::eTessellationEvaluation:
            kind = shaderc_glsl_tess_evaluation_shader;
            break;
        case vk::ShaderStageFlagBits::eGeometry:
            kind = shaderc_glsl_geometry_shader;
            break;
        case vk::ShaderStageFlagBits::eCompute:
            kind = shaderc_glsl_compute_shader;
            break;
        case vk::ShaderStageFlagBits::eAnyHitKHR:
            kind = shaderc_glsl_anyhit_shader;
            break;
        case vk::ShaderStageFlagBits::eCallableKHR:
            kind = shaderc_glsl_callable_shader;
            break;
        case vk::ShaderStageFlagBits::eIntersectionKHR:
            kind = shaderc_glsl_intersection_shader;
            break;
        case vk::ShaderStageFlagBits::eMissKHR:
            kind = shaderc_glsl_miss_shader;
            break;
        case vk::ShaderStageFlagBits::eRaygenKHR:
            kind = shaderc_glsl_raygen_shader;
            break;
        case vk::ShaderStageFlagBits::eClosestHitKHR:
            kind = shaderc_glsl_closesthit_shader;
            break;
        case vk::ShaderStageFlagBits::eMeshEXT:
            kind = shaderc_glsl_mesh_shader;
            break;
        case vk::ShaderStageFlagBits::eTaskEXT:
            kind = shaderc_glsl_task_shader;
            break;
        default:
            break;
        }


        shaderc::Compiler compiler;
        auto              res = compiler.CompileGlslToSpv(glsl, kind, "shader.glsl");

        if (res.GetNumErrors() != 0) {
            std::cerr << "Shader Compilation Error: " << res.GetErrorMessage() << std::endl;
            throw std::runtime_error("Failed to compile shader");
        }

        std::vector<uint32_t> spirv(res.begin(), res.end());
        return spirv;
    }

    vk::ShaderStageFlagBits infer_stage_from_path(const std::filesystem::path &path) {
        auto ext = path.extension().string();

        if (ext == ".vert") {
            return vk::ShaderStageFlagBits::eVertex;
        }

        if (ext == ".frag") {
            return vk::ShaderStageFlagBits::eFragment;
        }

        if (ext == ".geom") {
            return vk::ShaderStageFlagBits::eGeometry;
        }

        if (ext == ".tesc") {
            return vk::ShaderStageFlagBits::eTessellationControl;
        }

        if (ext == ".tese") {
            return vk::ShaderStageFlagBits::eTessellationEvaluation;
        }

        if (ext == ".comp") {
            return vk::ShaderStageFlagBits::eCompute;
        }


        return vk::ShaderStageFlagBits::eAll;
    }

    ShaderModule::ShaderModule(const std::shared_ptr<Context> &context, const ShaderModuleInfo &info) : m_context(context) {
        std::vector<uint32_t> spirv_code;
        if (info.source.index() == 1) {
            auto code_ = std::get<ShaderCode>(info.source);
            if (code_.code.index() == 0) {
                spirv_code = compile_glsl(std::get<std::string>(code_.code), info.stage);
            } else {
                spirv_code = std::get<std::vector<uint32_t>>(code_.code);
            }
        } else {
            auto path = std::get<std::filesystem::path>(info.source);

            if (info.type == ShaderModuleSourceType::GLSL) {
                std::string             glsl_source = read_file_text(path);
                vk::ShaderStageFlagBits stage       = info.stage;
                if (stage == vk::ShaderStageFlagBits::eAll) {
                    // try to infer from extension
                    stage = infer_stage_from_path(path);
                }

                spirv_code = compile_glsl(glsl_source, stage);
            } else { // assume spir-v
                spirv_code = read_file_binary(path);
            }
        }

        m_module = m_context->device().createShaderModule({{}, spirv_code});
    }

    std::shared_ptr<ShaderModule> ShaderModule::load(const std::shared_ptr<Context> &context, const ShaderModuleInfo &info) {
        return std::shared_ptr<ShaderModule>(new ShaderModule(context, info));
    }

    ShaderModule::~ShaderModule() {
        m_context->device().destroyShaderModule(m_module);
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_shader(const ShaderStageDefinition &def) {
        shader_stages.push_back(def);

        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_shader(vk::ShaderStageFlagBits stage, const ShaderModuleInfo &module) {
        shader_stages.push_back(ShaderStageDefinition{module, stage});

        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_shader(ShaderModuleSourceType source_type, vk::ShaderStageFlagBits stage, const ShaderModuleCodeSource &source) {
        shader_stages.push_back(ShaderStageDefinition{.module = ShaderModuleInfo{.source = source, .type = source_type, .stage = stage}, .stage = stage});

        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_glsl_shader(const std::filesystem::path &path) {
        const vk::ShaderStageFlagBits stage = infer_stage_from_path(path);

        shader_stages.push_back(ShaderStageDefinition{.module = ShaderModuleInfo{.source = path, .type = ShaderModuleSourceType::GLSL, .stage = stage}, .stage = stage});

        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_blend_attachment(const vk::PipelineColorBlendAttachmentState &blend_attachment) {
        color_blend_attachments.push_back(blend_attachment);
        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_dynamic_state(const vk::DynamicState state) {
        dynamic_states.insert(state);
        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_vertex_binding(uint32_t binding, uint32_t stride, vk::VertexInputRate input_rate) {
        vertex_bindings.emplace_back(binding, stride, input_rate);
        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_vertex_attribute(uint32_t binding, uint32_t location, vk::Format format, uint32_t offset) {
        vertex_attributes.emplace_back(location, binding, format, offset);
        return *this;
    }

    GraphicsPipelineBuilder &GraphicsPipelineBuilder::add_standard_blend_attachment() {
        color_blend_attachments.push_back(vk::PipelineColorBlendAttachmentState{
            true,
            vk::BlendFactor::eSrcAlpha,
            vk::BlendFactor::eOneMinusSrcAlpha,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        });

        return *this;
    }

    GraphicsPipeline::GraphicsPipeline(const std::shared_ptr<Context> &context, const GraphicsPipelineBuilder &builder) : m_context(context) {
        std::vector<vk::PipelineShaderStageCreateInfo> stages;

        for (const auto &sm : builder.shader_stages) {
            switch (sm.module.index()) {
            case 0: {
                stages.push_back(vk::PipelineShaderStageCreateInfo({}, sm.stage, std::get<0>(sm.module), "main"));
            } break;
            case 1: {
                m_shader_modules.push_back(std::get<1>(sm.module));
                stages.push_back(vk::PipelineShaderStageCreateInfo({}, sm.stage, std::get<1>(sm.module)->module(), "main"));
            } break;

            case 2: {
                auto mod = ShaderModule::load(m_context, std::get<2>(sm.module));
                m_shader_modules.push_back(mod);
                stages.push_back(vk::PipelineShaderStageCreateInfo({}, sm.stage, mod->module(), "main"));
            } break;
            default:
                throw std::runtime_error("Invalid shader module (variant incorrectly set)");
            }
        }

        std::vector<vk::DynamicState> dynamic_states(builder.dynamic_states.begin(), builder.dynamic_states.end());

        vk::PipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.setDynamicStates(dynamic_states);


        vk::PipelineVertexInputStateCreateInfo vertex_input_state{};
        vertex_input_state.setVertexAttributeDescriptions(builder.vertex_attributes);
        vertex_input_state.setVertexBindingDescriptions(builder.vertex_bindings);


        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{};
        input_assembly_state.setTopology(builder.primitive_topology);
        input_assembly_state.setPrimitiveRestartEnable(builder.enable_primitive_restart);


        vk::PipelineTessellationStateCreateInfo tessellation_state{};
        tessellation_state.setPatchControlPoints(builder.patch_control_points);


        vk::PipelineViewportStateCreateInfo viewport_state{};
        viewport_state.setViewports(builder.viewports);
        viewport_state.setScissors(builder.scissors); // TODO: adders for these two in the builder struct


        vk::PipelineRasterizationStateCreateInfo rasterization_state{};
        rasterization_state.setDepthClampEnable(builder.enable_depth_clamp);
        rasterization_state.setRasterizerDiscardEnable(builder.enable_rasterizer_discard);
        rasterization_state.setPolygonMode(builder.polygon_mode);
        rasterization_state.setCullMode(builder.cull_mode);
        rasterization_state.setFrontFace(builder.front_face);
        rasterization_state.setDepthBiasEnable(builder.enable_depth_bias);
        rasterization_state.setDepthBiasConstantFactor(builder.depth_bias_constant_factor);
        rasterization_state.setDepthBiasClamp(builder.depth_bias_clamp);
        rasterization_state.setDepthBiasSlopeFactor(builder.depth_bias_slope_factor);
        rasterization_state.setLineWidth(builder.line_width);


        vk::PipelineMultisampleStateCreateInfo multisample_state{};
        multisample_state.setRasterizationSamples(builder.rasterization_samples);
        multisample_state.setSampleShadingEnable(builder.enable_sample_shading);
        multisample_state.setMinSampleShading(builder.min_sample_shading);
        multisample_state.setPSampleMask(builder.sample_mask.empty() ? nullptr : builder.sample_mask.data());
        multisample_state.setAlphaToCoverageEnable(builder.enable_alpha_to_coverage);
        multisample_state.setAlphaToOneEnable(builder.enable_alpha_to_one);


        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.setDepthTestEnable(builder.enable_depth_test);
        depth_stencil_state.setDepthWriteEnable(builder.enable_depth_write);
        depth_stencil_state.setDepthCompareOp(builder.depth_compare_op);
        depth_stencil_state.setDepthBoundsTestEnable(builder.enable_depth_bounds_test);
        depth_stencil_state.setStencilTestEnable(builder.enable_stencil_test);
        depth_stencil_state.setFront(builder.stencil_front);
        depth_stencil_state.setBack(builder.stencil_back);
        depth_stencil_state.setMinDepthBounds(builder.min_depth_bounds);
        depth_stencil_state.setMaxDepthBounds(builder.max_depth_bounds);

        vk::PipelineColorBlendStateCreateInfo color_blend_state{};
        color_blend_state.setLogicOpEnable(builder.enable_logic_op);
        color_blend_state.setLogicOp(builder.logic_op);
        color_blend_state.setAttachments(builder.color_blend_attachments);
        color_blend_state.setBlendConstants(builder.blend_constants);

        vk::PipelineRenderingCreateInfo pipeline_rendering{};
        pipeline_rendering.setColorAttachmentFormats(builder.color_attachment_formats);
        pipeline_rendering.setDepthAttachmentFormat(builder.depth_format);
        pipeline_rendering.setStencilAttachmentFormat(builder.stencil_format);


        vk::GraphicsPipelineCreateInfo pipeline_create_info = {};
        pipeline_create_info.setStages(stages);
        pipeline_create_info.setPVertexInputState(&vertex_input_state);
        pipeline_create_info.setPInputAssemblyState(&input_assembly_state);
        pipeline_create_info.setPTessellationState(&tessellation_state);
        pipeline_create_info.setPViewportState(&viewport_state);
        pipeline_create_info.setPRasterizationState(&rasterization_state);
        pipeline_create_info.setPMultisampleState(&multisample_state);
        pipeline_create_info.setPDepthStencilState(&depth_stencil_state);
        pipeline_create_info.setPColorBlendState(&color_blend_state);
        pipeline_create_info.setPDynamicState(&dynamic_state);
        pipeline_create_info.setLayout(builder.layout->pipeline_layout());
        pipeline_create_info.setRenderPass(builder.render_pass);
        pipeline_create_info.setSubpass(builder.subpass);
        pipeline_create_info.setBasePipelineHandle(builder.base_pipeline);
        pipeline_create_info.setBasePipelineIndex(builder.base_pipeline_index);

        pipeline_create_info.pNext = &pipeline_rendering;

        m_pipeline =
            m_context->device().createGraphicsPipeline(m_context->pipeline_cache(), pipeline_create_info).value; // TODO: do something sort of checking on the actual result.
    }

    GraphicsPipeline::~GraphicsPipeline() {
        m_context->device().destroyPipeline(m_pipeline);
    }
} // namespace neuron::render
