cmake_minimum_required(VERSION 3.16)
project(VulkanRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include common macros file (check if path is correct)
include(${CMAKE_SOURCE_DIR}/CommonMacros.txt)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/External/imgui)

# ImGui source files (since there's no CMakeLists.txt in imgui directory)
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp  # Optional, for demo code
)

set(IMGUI_BACKEND_SRC
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Add the libs subdirectory
add_subdirectory(External/glfw)
add_subdirectory(External/glm)
add_subdirectory(External/vk-bootstrap)
add_subdirectory(External/VulkanMemoryAllocator)
add_subdirectory(External/assimp)

# Find all source files
file(GLOB SOURCES "Source/*.cpp")

# Add Vulkan
find_package(Vulkan REQUIRED)

# Create the executable
add_executable(VulkanRenderer ${SOURCES} ${IMGUI_SRC} ${IMGUI_BACKEND_SRC})


target_include_directories(VulkanRenderer PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    External/src/VulkanMemoryAllocator
    External/src/imgui
     ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/External/glfw/include
)

target_link_libraries(VulkanRenderer PRIVATE 
    glfw 
    glm 
    vk-bootstrap 
    VulkanMemoryAllocator 
    assimp 
    Vulkan::Vulkan
)