//
// Created by andy on 9/6/24.
//

#pragma once

#include "neuron/base.hpp"

namespace neuron::intfc {

    class NEURON_API SurfaceProvider {
      public:
        virtual ~SurfaceProvider() = default;

        [[nodiscard]] virtual vk::SurfaceKHR get_surface() const = 0;
    };

    class NEURON_API ExtentProvider {
        public:
        virtual ~ExtentProvider() = default;

        [[nodiscard]] virtual vk::Extent2D get_extent() const = 0;
    };


    // TODO: render target interface
} // namespace neuron::intfc
