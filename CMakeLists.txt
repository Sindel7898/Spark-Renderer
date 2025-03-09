cmake_minimum_required(VERSION 3.16)
project(VulkanRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include common macros file (check if path is correct)
include(${CMAKE_SOURCE_DIR}/CommonMacros.txt)

# Add the libs subdirectory
add_subdirectory(Bootstrapping/src/glfw)
add_subdirectory(Bootstrapping/src/glm)
add_subdirectory(Bootstrapping/src/vk-bootstrap)

# Find all source files
file(GLOB SOURCES "Source/*.cpp")

# Add Vulkan
find_package(Vulkan REQUIRED)

# Create the executable
add_executable(VulkanRenderer ${SOURCES})

# Include the src directory for headers
target_include_directories(VulkanRenderer PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link libraries
target_link_libraries(VulkanRenderer PRIVATE glfw glm vk-bootstrap Vulkan::Vulkan)
