cmake_minimum_required(VERSION 3.26)
project(Neuron VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(FetchContent)

find_package(Vulkan REQUIRED)

FetchContent_Declare(
        glfw3
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        7b6aead9fb88b3623e3b3725ebb42670cbe4c579
)

FetchContent_MakeAvailable(glfw3)

add_library(neuron src/neuron/neuron.cpp src/neuron/neuron.hpp
        src/neuron/neuron.cpp
        src/neuron/neuron.hpp
        src/neuron/base.hpp
        src/neuron/os/window.cpp
        src/neuron/os/window.hpp
        src/neuron/interface.hpp
        src/neuron/render/display_system.cpp
        src/neuron/render/display_system.hpp)
target_include_directories(neuron PUBLIC src/)
target_link_libraries(neuron PUBLIC Vulkan::Vulkan glfw)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(neuron PUBLIC -DNEURON_BUILD_SHARED)
    target_compile_definitions(neuron PRIVATE -DNEURON_BUILD_SHARED_EXPORT)
endif()

target_compile_definitions(neuron PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

target_compile_definitions(neuron PUBLIC NEURON_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} NEURON_VERSION_MINOR=${PROJECT_VERSION_MINOR} NEURON_VERSION_PATCH=${PROJECT_VERSION_PATCH} NEURON_VERSION_STRING="${PROJECT_VERSION}")

add_library(neuron::neuron ALIAS neuron)

add_subdirectory(example)