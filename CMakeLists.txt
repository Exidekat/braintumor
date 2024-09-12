cmake_minimum_required(VERSION 3.26)
project(Neuron VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(FetchContent)

message("$ENV{VULKAN_SDK}")

# Platform-specific Vulkan setup
if (WIN32)
    # Windows-specific Vulkan setup
    find_library(Vulkan_LIB NAMES vulkan-1 HINTS "$ENV{VULKAN_SDK}/Lib")
    if (Vulkan_LIB)
        add_library(Vulkan::Vulkan STATIC IMPORTED)
        set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION "${Vulkan_LIB}")
        target_include_directories(Vulkan::Vulkan INTERFACE "$ENV{VULKAN_SDK}/Include")
    else()
        message(FATAL_ERROR "Could not find Vulkan on Windows! Make sure VULKAN_SDK is set correctly.")
    endif()

    if (BUILD_SHARED_LIBS)
        add_library(Vulkan::shaderc SHARED IMPORTED)
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Bin/shaderc_shared.dll")
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_IMPLIB "$ENV{VULKAN_SDK}/Lib/shaderc_shared.lib")
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")
        target_include_directories(Vulkan::shaderc INTERFACE "$ENV{VULKAN_SDK}/Include")
    else()
        add_library(Vulkan::shaderc STATIC IMPORTED)
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Lib/shaderc_combined.lib")
        target_include_directories(Vulkan::shaderc INTERFACE "$ENV{VULKAN_SDK}/Include")
    endif()

elseif(APPLE)
    # macOS-specific Vulkan setup
    find_library(Vulkan_LIB NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib")
    find_path(Vulkan_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include")

    if(Vulkan_LIB AND Vulkan_INCLUDE_DIR)
        message(STATUS "Found Vulkan library: ${Vulkan_LIB}")
        message(STATUS "Found Vulkan include directory: ${Vulkan_INCLUDE_DIR}")
        add_library(Vulkan::Vulkan SHARED IMPORTED)
        set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION "${Vulkan_LIB}")
        target_include_directories(Vulkan::Vulkan INTERFACE "${Vulkan_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "Could not find Vulkan on macOS! Make sure VULKAN_SDK is set correctly.")
    endif()

elseif(UNIX AND NOT APPLE)
    # Linux-specific Vulkan setup
    find_library(Vulkan_LIB NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib")
    find_path(Vulkan_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include")

    if(Vulkan_LIB AND Vulkan_INCLUDE_DIR)
        message(STATUS "Found Vulkan library: ${Vulkan_LIB}")
        message(STATUS "Found Vulkan include directory: ${Vulkan_INCLUDE_DIR}")
        add_library(Vulkan::Vulkan SHARED IMPORTED)
        set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION "${Vulkan_LIB}")
        target_include_directories(Vulkan::Vulkan INTERFACE "${Vulkan_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "Could not find Vulkan on Linux! Make sure VULKAN_SDK is set correctly.")
    endif()

    find_library(Shaderc_LIB NAMES libshaderc_combined.a HINTS "$ENV{VULKAN_SDK}/lib")
    if(Shaderc_LIB)
        add_library(Vulkan::shaderc STATIC IMPORTED)
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "${Shaderc_LIB}")
        target_include_directories(Vulkan::shaderc INTERFACE "$ENV{VULKAN_SDK}/include")
    endif()
endif()

# Fetch GLFW, GLM, and VulkanMemoryAllocator using FetchContent
FetchContent_Declare(
    glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        7b6aead9fb88b3623e3b3725ebb42670cbe4c579
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0af55ccecd98d4e5a8d1fad7de25ba429d60e863
)

FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        009ecd192c1289c7529bff248a16cfe896254816
)

FetchContent_MakeAvailable(glfw3 glm VulkanMemoryAllocator)

# Define the neuron library
add_library(neuron
    src/neuron/neuron.cpp src/neuron/neuron.hpp
    src/neuron/base.hpp
    src/neuron/os/window.cpp src/neuron/os/window.hpp
    src/neuron/interface.hpp
    src/neuron/render/display_system.cpp src/neuron/render/display_system.hpp
    src/neuron/render/simple_render_pass.cpp src/neuron/render/simple_render_pass.hpp
    src/neuron/render/graphics_pipeline.cpp src/neuron/render/graphics_pipeline.hpp
    src/neuron/render/pipeline_layout.cpp src/neuron/render/pipeline_layout.hpp
)

# Link libraries to the neuron target
target_include_directories(neuron PUBLIC src/)
target_link_libraries(neuron PUBLIC Vulkan::Vulkan glfw glm::glm GPUOpen::VulkanMemoryAllocator)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(neuron PUBLIC -DNEURON_BUILD_SHARED)
    target_compile_definitions(neuron PRIVATE -DNEURON_BUILD_SHARED_EXPORT)
endif()

target_compile_definitions(neuron PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Define Neuron version
target_compile_definitions(neuron PUBLIC
    NEURON_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NEURON_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NEURON_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    NEURON_VERSION_STRING="${PROJECT_VERSION}"
)

add_library(neuron::neuron ALIAS neuron)

# Add subdirectory for example
add_subdirectory(example)

# Prepare runtime directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/run)

add_custom_target(run_prep
    COMMAND ${CMAKE_COMMAND} -E copy -t ${CMAKE_SOURCE_DIR}/run $<TARGET_RUNTIME_DLLS:example> $<TARGET_FILE:example>
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/res/ ${CMAKE_SOURCE_DIR}/run/res
    COMMAND_EXPAND_LISTS
)

