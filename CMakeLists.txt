cmake_minimum_required(VERSION 3.16)
project(Spark)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/MP)
endif()

# --- 1. Define DLSS Path ---
set(DLSS_DIR ${CMAKE_SOURCE_DIR}/External/DLSS)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/External/imgui)
set(ImGuizmo_DIR ${CMAKE_SOURCE_DIR}/External/ImGuizmo)

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

# Subdirectories
add_subdirectory(External/glfw)
add_subdirectory(External/glm)
add_subdirectory(External/vk-bootstrap)
add_subdirectory(External/VulkanMemoryAllocator)
add_subdirectory(External/tracy)
add_subdirectory(External/fastgltf)
add_subdirectory(External/meshoptimizer)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "Source/*.cpp" "Source/*.c")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "Source/*.hpp" "Source/*.h")

add_executable(Spark
    ${SOURCES}
    ${HEADERS}
    ${TRACY_SOURCES}
    ${IMGUI_SRC}
    ${IMGUI_BACKEND_SRC}
    ${ImGuizmo_SRC}
)

source_group("Source Files" FILES ${SOURCES})
source_group("Header Files" FILES ${HEADERS})

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
    ${DLSS_DIR}/include
)

target_compile_definitions(Spark PRIVATE 
    TRACY_ENABLE
    NOMINMAX
)


set(DLSS_LIB_DIR ${DLSS_DIR}/lib/Windows_x86_64/x64)

target_link_libraries(Spark PRIVATE 
    glfw 
    glm 
    vk-bootstrap 
    VulkanMemoryAllocator
    fastgltf
    meshoptimizer
    Vulkan::Vulkan
    debug     "${DLSS_LIB_DIR}/nvsdk_ngx_d_dbg.lib"
    optimized "${DLSS_LIB_DIR}/nvsdk_ngx_d.lib"
)


add_custom_command(TARGET Spark POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DLSS_DIR}/lib/Windows_x86_64/dev/nvngx_dlss.dll"
    $<TARGET_FILE_DIR:Spark>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DLSS_DIR}/lib/Windows_x86_64/dev/nvngx_dlssd.dll"
    $<TARGET_FILE_DIR:Spark>
    COMMENT "Copying DLSS (SR & RR) DLLs to output directory"
)