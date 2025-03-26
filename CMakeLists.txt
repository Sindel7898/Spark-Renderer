cmake_minimum_required(VERSION 3.16)
project(VulkanRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include common macros file (check if path is correct)
include(${CMAKE_SOURCE_DIR}/CommonMacros.txt)

# ImGui source files (since there's no CMakeLists.txt in imgui directory)
set(IMGUI_SRC
    Bootstrapping/src/imgui/imgui.cpp
    Bootstrapping/src/imgui/imgui_draw.cpp
    Bootstrapping/src/imgui/imgui_tables.cpp
    Bootstrapping/src/imgui/imgui_widgets.cpp
    Bootstrapping/src/imgui/backends/imgui_impl_glfw.h
    Bootstrapping/src/imgui/backends/imgui_impl_glfw.cpp
    Bootstrapping/src/imgui/backends/imgui_impl_vulkan.h
    Bootstrapping/src/imgui/backends/imgui_impl_vulkan.cpp
)

# Add the libs subdirectory
add_subdirectory(Bootstrapping/src/glfw)
add_subdirectory(Bootstrapping/src/glm)
add_subdirectory(Bootstrapping/src/vk-bootstrap)
add_subdirectory(Bootstrapping/src/VulkanMemoryAllocator)
add_subdirectory(Bootstrapping/src/assimp)

# Find all source files
file(GLOB SOURCES "Source/*.cpp")

# Add Vulkan
find_package(Vulkan REQUIRED)

# Create the executable
add_executable(VulkanRenderer ${SOURCES} ${IMGUI_SRC})


target_include_directories(VulkanRenderer PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    Bootstrapping/src/VulkanMemoryAllocator
    Bootstrapping/src/imgui
)

# Link libraries
target_link_libraries(VulkanRenderer PRIVATE glfw glm vk-bootstrap VulkanMemoryAllocator assimp Vulkan::Vulkan)
