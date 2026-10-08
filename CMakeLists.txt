cmake_minimum_required(VERSION 3.22.1)
project(Aether3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AETHER_ENABLE_VALIDATION "Enable Vulkan validation layers" OFF)

if(ANDROID)
    find_library(AETHER_VULKAN_LIBRARY vulkan)
    find_library(AETHER_LOG_LIBRARY log)
    find_library(AETHER_ANDROID_LIBRARY android)

    add_library(aether3d SHARED
        native/aether_render/src/forward_plus_renderer.cpp
    )

    target_include_directories(aether3d PRIVATE
        native/aether_render/include
    )

    target_link_libraries(aether3d PRIVATE
        ${AETHER_VULKAN_LIBRARY}
        ${AETHER_LOG_LIBRARY}
        ${AETHER_ANDROID_LIBRARY}
    )

    target_compile_definitions(aether3d PRIVATE
        VK_USE_PLATFORM_ANDROID_KHR=1
        $<$<BOOL:${AETHER_ENABLE_VALIDATION}>:AETHER_VULKAN_VALIDATION=1>
    )

    target_compile_options(aether3d PRIVATE -Wall -Wextra -Wpedantic -Werror)
else()
    find_package(Vulkan REQUIRED)

    add_library(aether3d STATIC
        native/aether_render/src/forward_plus_renderer.cpp
    )

    target_include_directories(aether3d PUBLIC
        native/aether_render/include
    )

    target_link_libraries(aether3d PUBLIC Vulkan::Vulkan)
endif()
