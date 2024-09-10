//
// Created by andy on 9/9/24.
//

#include "pipeline_layout.hpp"


namespace neuron::render {
    PipelineLayout::PipelineLayout(const std::shared_ptr<Context> &context) : m_context(context) {
        m_pipeline_layout = m_context->device().createPipelineLayout({});
    }

    PipelineLayout::~PipelineLayout() {
        m_context->device().destroy(m_pipeline_layout);
    }
} // namespace neuron::render
