#pragma once

#ifdef WIN32
#ifdef NEURON_BUILD_SHARED
#ifdef NEURON_BUILD_SHARED_EXPORT
#define NEURON_API __declspec(dllexport)
#else
#define NEURON_API __declspec(dllimport)
#endif
#else
#define NEURON_API
#endif
#else
#define NEURON_API
#endif


#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

