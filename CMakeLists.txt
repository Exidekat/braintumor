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
else() # Linux
  find_path(Vulkan_INCLUDE NAMES vulkan/vulkan.hpp)
  find_path(Shaderc_INCLUDE NAMES shaderc/shaderc.hpp)


  find_library(Vulkan_LIB NAMES libvulkan.so)
  add_library(Vulkan::Vulkan SHARED IMPORTED)
  set_property(TARGET Vulkan::Vulkan PROPERTY IMPORTED_LOCATION "${Vulkan_LIB}")

  if (BUILD_SHARED_LIBS)


    find_library(Shaderc_LIB NAMES libshaderc_shared.so libshaderc.so)

    add_library(Vulkan::shaderc SHARED IMPORTED)
    set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "${Shaderc_LIB}")

  else()
    find_library(Shaderc_LIB NAMES libshaderc_combined.a)

    add_library(Vulkan::shaderc STATIC IMPORTED)
    set_property(TARGET Vulkan::shaderc PROPERTY IMPORTED_LOCATION "${Shaderc_LIB}")
  endif()

  target_include_directories(Vulkan::Vulkan INTERFACE "${Vulkan_INCLUDE}")
  target_include_directories(Vulkan::shaderc INTERFACE "${Shaderc_INCLUDE}")
endif()



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

FetchContent_MakeAvailable(glfw3 glm)

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
        src/neuron/render/pipeline_layout.cpp
        src/neuron/render/pipeline_layout.hpp
)
target_include_directories(neuron PUBLIC src/)
target_link_libraries(neuron PUBLIC Vulkan::Vulkan Vulkan::shaderc glfw glm::glm)


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
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/res/ ${CMAKE_SOURCE_DIR}/run/res
    COMMAND_EXPAND_LISTS)
