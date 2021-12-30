cmake_minimum_required(VERSION 3.20)
project(VulkanProject_I)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(glfw)
add_subdirectory(glm)

set (HEADERS
    VulkanRenderer.h
    Utilities.h
)

set (SOURCES
    VulkanRenderer.cpp
    main.cpp
)

add_executable(VulkanProject_I ${SOURCES})

target_include_directories(VulkanProject_I PUBLIC
    glfw/include
    glm/glm
    /Users/elyxAir/VulkanSDK/1.2.198.1/MoltenVK/include
)

find_package(Vulkan REQUIRED FATAL_ERROR)
target_link_libraries(VulkanProject_I PUBLIC
    glfw
    glm
    Vulkan::Vulkan
)
