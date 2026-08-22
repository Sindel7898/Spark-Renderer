cmake_minimum_required(VERSION 3.16)
project(Spark)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/MP)
endif()

# ------------------------------------------------------------
# External Library Paths
# ------------------------------------------------------------

set(DLSS_DIR ${CMAKE_SOURCE_DIR}/External/DLSS)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/External/imgui)
set(IMGUI_BACKENDS_DIR ${IMGUI_DIR}/backends)

set(ImGuizmo_DIR ${CMAKE_SOURCE_DIR}/External/ImGuizmo)
set(IMPLOT_DIR ${CMAKE_SOURCE_DIR}/External/implot)
set(RAPIDYAML_DIR ${CMAKE_SOURCE_DIR}/External/rapidyaml)

set(NVPERF_DIR ${CMAKE_SOURCE_DIR}/External/NVIDIA_Nsight_Perf)
set(NVPERF_NVPERF_DIR ${NVPERF_DIR}/NvPerf)
set(NVPERF_REDIST_DIR ${NVPERF_DIR}/redist)

# ------------------------------------------------------------
# NvPerf
# ------------------------------------------------------------

set(NVPERF_INCLUDE_DIRS
    ${NVPERF_NVPERF_DIR}/include
    ${NVPERF_REDIST_DIR}/NvPerfUtility/include
    ${NVPERF_REDIST_DIR}/include/windows-desktop-x64
)

set(NVPERF_LIB
    ${NVPERF_NVPERF_DIR}/bin/x64/nvperf_grfx_host.lib
)

# ------------------------------------------------------------
# DLSS
# ------------------------------------------------------------

set(DLSS_LIB_DIR
    ${DLSS_DIR}/lib/Windows_x86_64/x64
)

# ------------------------------------------------------------
# ImGui
# ------------------------------------------------------------

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
)

set(IMGUI_BACKEND_SRC
    ${IMGUI_BACKENDS_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_BACKENDS_DIR}/imgui_impl_vulkan.cpp
)

# ------------------------------------------------------------
# ImGuizmo
# ------------------------------------------------------------

set(ImGuizmo_SRC
    ${ImGuizmo_DIR}/GraphEditor.cpp
    ${ImGuizmo_DIR}/ImCurveEdit.cpp
    ${ImGuizmo_DIR}/ImGradient.cpp
    ${ImGuizmo_DIR}/ImGuizmo.cpp
    ${ImGuizmo_DIR}/ImSequencer.cpp
)

# ------------------------------------------------------------
# ImPlot
# ------------------------------------------------------------

set(IMPLOT_SRC
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
    ${IMPLOT_DIR}/implot_demo.cpp
)

# ------------------------------------------------------------
# Tracy
# ------------------------------------------------------------

set(TRACY_SOURCES
    ${CMAKE_SOURCE_DIR}/External/tracy/public/TracyClient.cpp
)

# ------------------------------------------------------------
# External Dependencies
# ------------------------------------------------------------

add_subdirectory(External/glfw)
add_subdirectory(External/glm)
add_subdirectory(External/vk-bootstrap)
add_subdirectory(External/VulkanMemoryAllocator)
add_subdirectory(External/tracy)
add_subdirectory(External/fastgltf)
add_subdirectory(External/meshoptimizer)

find_package(Vulkan REQUIRED)

# ------------------------------------------------------------
# Engine Source Files
# ------------------------------------------------------------

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "Source/*.cpp"
    "Source/*.c"
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "Source/*.hpp"
    "Source/*.h"
)

# ------------------------------------------------------------
# Executable
# ------------------------------------------------------------

add_executable(Spark
    ${SOURCES}
    ${HEADERS}
    ${TRACY_SOURCES}
    ${IMGUI_SRC}
    ${IMGUI_BACKEND_SRC}
    ${ImGuizmo_SRC}
    ${IMPLOT_SRC}
)

# ------------------------------------------------------------
# Includes
# ------------------------------------------------------------

target_include_directories(Spark PRIVATE

    ${CMAKE_SOURCE_DIR}/src

    ${IMGUI_DIR}
    ${IMGUI_BACKENDS_DIR}

    ${ImGuizmo_DIR}
    ${IMPLOT_DIR}
    ${RAPIDYAML_DIR}

    ${CMAKE_SOURCE_DIR}/External/glfw/include
    ${CMAKE_SOURCE_DIR}/External/tracy/public/tracy
    ${CMAKE_SOURCE_DIR}/External/VulkanMemoryAllocator/include

    ${DLSS_DIR}/include
    ${NVPERF_INCLUDE_DIRS}

    ${Vulkan_INCLUDE_DIRS}
)

# ------------------------------------------------------------
# Compile Definitions
# ------------------------------------------------------------

target_compile_definitions(Spark PRIVATE
    TRACY_ENABLE
    NOMINMAX
)

# ------------------------------------------------------------
# Link Libraries
# ------------------------------------------------------------

target_link_libraries(Spark PRIVATE

    glfw
    glm
    vk-bootstrap
    VulkanMemoryAllocator
    fastgltf
    meshoptimizer

    Vulkan::Vulkan

    ${NVPERF_LIB}

    debug     "${DLSS_LIB_DIR}/nvsdk_ngx_d_dbg.lib"
    optimized "${DLSS_LIB_DIR}/nvsdk_ngx_d.lib"
)

# ------------------------------------------------------------
# Visual Studio Debugger Working Directory
# ------------------------------------------------------------

set_target_properties(Spark PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/exe"
)

# ------------------------------------------------------------
# Post Build Copy DLLs & Executable to exe/
# ------------------------------------------------------------

add_custom_command(TARGET Spark POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DLSS_DIR}/lib/Windows_x86_64/rel/nvngx_dlss.dll"
    $<TARGET_FILE_DIR:Spark>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DLSS_DIR}/lib/Windows_x86_64/rel/nvngx_dlssd.dll"
    $<TARGET_FILE_DIR:Spark>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DLSS_DIR}/lib/Windows_x86_64/rel/nvngx_dlssg.dll"
    $<TARGET_FILE_DIR:Spark>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${NVPERF_NVPERF_DIR}/bin/x64/nvperf_grfx_host.dll"
    $<TARGET_FILE_DIR:Spark>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Spark>
    "${CMAKE_SOURCE_DIR}/exe/Spark.exe"

    COMMENT "Copying DLSS + NvPerf DLLs and Spark.exe to exe/"
)