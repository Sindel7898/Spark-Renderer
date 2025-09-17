cmake_minimum_required(VERSION 3.16)
project(Spark)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set paths for external libraries
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/External/imgui)
set(ImGuizmo_DIR ${CMAKE_SOURCE_DIR}/External/ImGuizmo)

# ImGui and other external source files
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)
set(ImGuizmo_SRC
    ${ImGuizmo_DIR}/GraphEditor.cpp
    ${ImGuizmo_DIR}/ImCurveEdit.cpp
    ${ImGuizmo_DIR}/ImGradient.cpp
    ${ImGuizmo_DIR}/ImGuizmo.cpp
    ${ImGuizmo_DIR}/ImSequencer.cpp  
)
set(IMGUI_BACKEND_SRC
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
set(TRACY_SOURCES ${CMAKE_SOURCE_DIR}/External/tracy/public/TracyClient.cpp)

# Add dependencies from subdirectories
add_subdirectory(External/glfw)
add_subdirectory(External/glm)
add_subdirectory(External/vk-bootstrap)
add_subdirectory(External/VulkanMemoryAllocator)
add_subdirectory(External/tracy)
add_subdirectory(External/fastgltf)
add_subdirectory(External/meshoptimizer)

# Find packages
find_package(Vulkan REQUIRED)

# --- This is the correct way to discover your project files ---
file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    "Source/*.cpp"
    "Source/*.c"
)
file(GLOB_RECURSE HEADERS
    CONFIGURE_DEPENDS
    "Source/*.hpp"
    "Source/*.h"
)

# --- This is the single, correct place to define the executable ---
# It combines your project's sources with all the external library sources.
add_executable(Spark
    ${SOURCES}
    ${HEADERS}
    ${TRACY_SOURCES}
    ${IMGUI_SRC}
    ${IMGUI_BACKEND_SRC}
    ${ImGuizmo_SRC}
)

# Create the filters for Visual Studio
source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})

# Add include directories to the target
target_include_directories(Spark PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    External/src/VulkanMemoryAllocator
    External/src/imgui
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    External/ImGuizmo
    ${ImGuizmo_DIR}
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/External/glfw/include
    ${CMAKE_SOURCE_DIR}/External/tracy/public/tracy
)

# Add compile definitions to the target
target_compile_definitions(Spark PRIVATE TRACY_ENABLE)

# Link libraries to the target
target_link_libraries(Spark PRIVATE 
    glfw 
    glm 
    vk-bootstrap 
    VulkanMemoryAllocator
    fastgltf
    meshoptimizer
    Vulkan::Vulkan
)