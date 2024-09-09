cmake_minimum_required(VERSION 3.26)
project(Neuron VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(FetchContent)


message("$ENV{VULKAN_SDK}")

# find vulkan sdk
if (WIN32)
    add_library(Vulkan::Vulkan STATIC IMPORTED)
    set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
    target_include_directories(Vulkan::Vulkan INTERFACE "$ENV{VULKAN_SDK}/Include/")

    if (BUILD_SHARED_LIBS)
        add_library(Vulkan::shaderc STATIC IMPORTED)
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
else() # Linux
    if (BUILD_SHARED_LIBS)
        find_library(Vulkan_LIB NAMES libvulkan.so)
        find_library(Vulkan_LIB NAMES libshaderc_shared.so)

        add_library(Vulkan::Vulkan STATIC IMPORTED)
        set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/lib/libvulkan.so")
        target_include_directories(Vulkan::Vulkan INTERFACE "$ENV{VULKAN_SDK}/include/")

        add_library(Vulkan::shaderc SHARED IMPORTED)
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/lib/shaderc_shared.so")
        target_include_directories(Vulkan::shaderc INTERFACE "$ENV{VULKAN_SDK}/include/")
    else()
        find_library(Vulkan_LIB NAMES libvulkan.a)
        find_library(Vulkan_LIB NAMES libshaderc_combined.a)

        find_path

        add_library(Vulkan::Vulkan STATIC IMPORTED)
        set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION )
        target_include_directories(Vulkan::Vulkan INTERFACE "$ENV{VULKAN_SDK}/include/")

        add_library(Vulkan::shaderc STATIC IMPORTED)
        set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "$ENV{VULKAN_SDK}/lib/libshaderc_combined.a")
        target_include_directories(Vulkan::shaderc INTERFACE "$ENV{VULKAN_SDK}/include/")
    endif()
endif()



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
        src/neuron/render/display_system.hpp
        src/neuron/render/simple_render_pass.cpp
        src/neuron/render/simple_render_pass.hpp
        src/neuron/render/graphics_pipeline.cpp
        src/neuron/render/graphics_pipeline.hpp
)
target_include_directories(neuron PUBLIC src/)
target_link_libraries(neuron PUBLIC Vulkan::Vulkan Vulkan::shaderc glfw)


if (BUILD_SHARED_LIBS)
    target_compile_definitions(neuron PUBLIC -DNEURON_BUILD_SHARED)
    target_compile_definitions(neuron PRIVATE -DNEURON_BUILD_SHARED_EXPORT)
endif()

target_compile_definitions(neuron PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

target_compile_definitions(neuron PUBLIC NEURON_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} NEURON_VERSION_MINOR=${PROJECT_VERSION_MINOR} NEURON_VERSION_PATCH=${PROJECT_VERSION_PATCH} NEURON_VERSION_STRING="${PROJECT_VERSION}")

add_library(neuron::neuron ALIAS neuron)

add_subdirectory(example)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/run)

add_custom_target(run_prep
    COMMAND ${CMAKE_COMMAND} -E copy -t ${CMAKE_SOURCE_DIR}/run $<TARGET_RUNTIME_DLLS:example> $<TARGET_FILE:example>
    COMMAND_EXPAND_LISTS)