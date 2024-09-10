//
// Created by andy on 9/9/24.
//

#pragma once

#include "neuron/base.hpp"
#include "neuron/neuron.hpp"

#include <memory>

namespace neuron::render {

    class NEURON_API PipelineLayout {
      public:
        explicit PipelineLayout(const std::shared_ptr<Context> &context);

        ~PipelineLayout();

        [[nodiscard]] vk::PipelineLayout pipeline_layout() const { return m_pipeline_layout; }

      private:

        std::shared_ptr<Context> m_context;
        vk::PipelineLayout m_pipeline_layout;
    };

} // namespace neuron::render
