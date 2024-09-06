//
// Created by andy on 9/6/24.
//

#pragma once

#include "neuron/base.hpp"

namespace neuron::intfc {

    class SurfaceProvider NEURON_API {
      public:
        virtual ~SurfaceProvider() = default;

        [[nodiscard]] virtual vk::SurfaceKHR get_surface() const = 0;
    };

    class ExtentProvider NEURON_API {
        public:
        virtual ~ExtentProvider() = default;

        [[nodiscard]] virtual vk::Extent2D get_extent() const = 0;
    };
} // namespace neuron::intfc
