cmake_minimum_required(VERSION 3.22)
project(aether3d LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(aether3d SHARED
    aether_jni.cpp
    aether_engine.cpp
    aether_vulkan.cpp
)
target_compile_options(aether3d PRIVATE -Wall -Wextra -Werror -Wno-missing-field-initializers -fno-exceptions -fno-rtti)
target_compile_definitions(aether3d PRIVATE VK_USE_PLATFORM_ANDROID_KHR=1)
find_library(log-lib log)
find_library(android-lib android)
find_library(vulkan-lib vulkan)
target_link_libraries(aether3d PRIVATE ${android-lib} ${log-lib} ${vulkan-lib})
