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

namespace neuron {
    class Context NEURON_API;


    template<typename T, typename D>
    concept pointer_satisfy = std::is_pointer_v<T> && (std::derived_from<std::remove_cvref_t<std::remove_pointer_t<T>>, D> || std::same_as<std::remove_cvref_t<std::remove_pointer_t<T>>, D>);

    template<typename T, typename D>
    concept reference_satisfy = std::is_reference_v<T> && (std::derived_from<std::remove_cvref_t<T>, D> || std::same_as<std::remove_cvref_t<T>, D>);


    template <typename T, typename R>
    concept pointer_like = pointer_satisfy<T, R> || requires (T t)
    {
        { t.operator->() } -> pointer_satisfy<R>;
        { t.operator*() } -> reference_satisfy<R>;
    };
} // namespace neuron
