cmake_minimum_required(VERSION 3.26)
project(Neuron VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(FetchContent)

message("VULKAN_SDK: $ENV{VULKAN_SDK}")

# Platform-specific Vulkan setup
if (WIN32)
    add_library(Vulkan::Vulkan STATIC IMPORTED)
    set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
    target_include_directories(Vulkan::Vulkan INTERFACE "$ENV{VULKAN_SDK}/Include/")

    if (BUILD_SHARED_LIBS)
        add_library(Vulkan::shaderc SHARED IMPORTED)
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Bin/shaderc_shared.dll")
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION_DEBUG "$ENV{VULKAN_SDK}/Bin/shaderc_sharedd.dll")
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_IMPLIB "$ENV{VULKAN_SDK}/Lib/shaderc_shared.lib")
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_IMPLIB_DEBUG "$ENV{VULKAN_SDK}/Lib/shaderc_sharedd.lib")
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")
        target_include_directories(Vulkan::shaderc INTERFACE "$ENV{VULKAN_SDK}/Include/")
    else()
        add_library(Vulkan::shaderc STATIC IMPORTED)
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Lib/shaderc_combined.lib")
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION_DEBUG "$ENV{VULKAN_SDK}/Lib/shaderc_combinedd.lib")
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_CONFIGURATIONS "RELEASE;DEBUG")
        target_include_directories(Vulkan::shaderc INTERFACE "$ENV{VULKAN_SDK}/Include/")
    endif()
else() # Linux and other non-WIN32 systems
    # Find Vulkan include directory
    find_path(Vulkan_INCLUDE 
        NAMES vulkan/vulkan.hpp vulkan/vulkan.h
        PATHS "$ENV{VULKAN_SDK}/include" "/usr/include" "/usr/local/include"
        NO_DEFAULT_PATH
    )

    if(NOT Vulkan_INCLUDE)
        message(FATAL_ERROR "Vulkan include directory not found. Please ensure Vulkan SDK is installed and VULKAN_SDK environment variable is set correctly.")
    else()
        message(STATUS "Vulkan include directory found at: ${Vulkan_INCLUDE}")
    endif()

    # Find Vulkan library
    find_library(Vulkan_LIB 
        NAMES vulkan libvulkan.so libvulkan.so.1 libvulkan.so.1.2
        PATHS "$ENV{VULKAN_SDK}/lib" "/usr/lib" "/usr/local/lib" 
        NO_DEFAULT_PATH
    )

    if(NOT Vulkan_LIB)
        message(FATAL_ERROR "Vulkan library not found. Please ensure Vulkan SDK is installed and VULKAN_SDK environment variable is set correctly.")
    else()
        message(STATUS "Vulkan library found at: ${Vulkan_LIB}")
    endif()

    # Find Shaderc include directory
    find_path(Shaderc_INCLUDE 
        NAMES shaderc/shaderc.hpp 
        PATHS "$ENV{VULKAN_SDK}/include" "/usr/include" "/usr/local/include"
        NO_DEFAULT_PATH
    )

    if(NOT Shaderc_INCLUDE)
        message(FATAL_ERROR "Shaderc include directory not found. Please ensure Shaderc is installed and VULKAN_SDK environment variable is set correctly.")
    else()
        message(STATUS "Shaderc include directory found at: ${Shaderc_INCLUDE}")
    endif()

    # Find Shaderc library
    find_library(Shaderc_LIB 
        NAMES shaderc_shared shaderc libshaderc.so libshaderc_shared.so libshaderc_combined.a 
        PATHS "$ENV{VULKAN_SDK}/lib" "/usr/lib" "/usr/local/lib" 
        NO_DEFAULT_PATH
    )

    if(NOT Shaderc_LIB)
        message(FATAL_ERROR "Shaderc library not found. Please ensure Shaderc is installed and VULKAN_SDK environment variable is set correctly.")
    else()
        message(STATUS "Shaderc library found at: ${Shaderc_LIB}")
    endif()

    # Define Vulkan::Vulkan Imported Target
    add_library(Vulkan::Vulkan SHARED IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES 
        IMPORTED_LOCATION "${Vulkan_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE}"
    )

    # Define Vulkan::shaderc Imported Target
    if (BUILD_SHARED_LIBS)
        add_library(Vulkan::shaderc SHARED IMPORTED)
    else()
        add_library(Vulkan::shaderc STATIC IMPORTED)
    endif()

    set_target_properties(Vulkan::shaderc PROPERTIES 
        IMPORTED_LOCATION "${Shaderc_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${Shaderc_INCLUDE}"
    )
endif()

# FetchContent declarations
FetchContent_Declare(
    glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579
)

FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG 009ecd192c1289c7529bff248a16cfe896254816
)

FetchContent_MakeAvailable(glfw3 VulkanMemoryAllocator)

# Handle shared libraries for Windows
if (WIN32)
    set(BSL_ ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
endif()

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 33b4a621a697a305bc3a7610d290677b96beb181
)

FetchContent_MakeAvailable(glm)

if (WIN32)
    set(BUILD_SHARED_LIBS ${BSL_})
endif()

# Define the neuron library FIRST
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

# After defining neuron, link libraries to the neuron target
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

# Option to switch between shared or static shaderc for mac sufferers
if(APPLE)
    if(USE_STATIC_SHADERC)
        message(STATUS "Using static shaderc library")
        # Link using Vulkan::shaderc instead of undefined variables
        target_link_libraries(neuron PRIVATE Vulkan::shaderc)
    else()
        message(STATUS "Using shared (dynamic) shaderc library")
        # Link using Vulkan::shaderc instead of undefined variables
        target_link_libraries(neuron PRIVATE Vulkan::shaderc)
    endif()
endif()

# **Ensure shaderc is linked on non-WIN32, non-APPLE systems**
if(NOT WIN32 AND NOT APPLE)
    target_link_libraries(neuron PUBLIC Vulkan::shaderc)
endif()

# Add subdirectory for example
add_subdirectory(example)

# Prepare runtime directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/run)

add_custom_target(run_prep
    COMMAND ${CMAKE_COMMAND} -E copy -t ${CMAKE_SOURCE_DIR}/run $<TARGET_RUNTIME_DLLS:example> $<TARGET_FILE:example>
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/res/ ${CMAKE_SOURCE_DIR}/run/res
    COMMAND_EXPAND_LISTS
)

