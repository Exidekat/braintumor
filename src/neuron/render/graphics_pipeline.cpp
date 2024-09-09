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

        auto  file_size = file.tellg();
        auto *buffer    = new char[file_size];
        file.seekg(0);
        file.read(buffer, file_size);
        file.close();
        std::string content(buffer, file_size);
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
        auto              res = compiler.CompileGlslToSpv(glsl, kind, "");

        if (res.GetNumErrors() != 0) {
            std::cerr << "Shader Compilation Error: " << res.GetErrorMessage() << std::endl;
            throw std::runtime_error("Failed to compile shader");
        }

        std::vector<uint32_t> spirv(res.begin(), res.end());
        return spirv;
    }

    vk::ShaderStageFlagBits infer_stage_from_path(const std::filesystem::path &path) {
        auto ext = path.extension().string();

        if (ext == "vert") {
            return vk::ShaderStageFlagBits::eVertex;
        }

        if (ext == "frag") {
            return vk::ShaderStageFlagBits::eFragment;
        }

        if (ext == "geom") {
            return vk::ShaderStageFlagBits::eGeometry;
        }

        if (ext == "tesc") {
            return vk::ShaderStageFlagBits::eTessellationControl;
        }

        if (ext == "tese") {
            return vk::ShaderStageFlagBits::eTessellationEvaluation;
        }

        if (ext == "comp") {
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

        
    }

    GraphicsPipeline::~GraphicsPipeline() {}
} // namespace neuron::render
