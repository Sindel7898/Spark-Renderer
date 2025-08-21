cmake_minimum_required(VERSION 3.16)
project(Spark)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include common macros file
include(${CMAKE_SOURCE_DIR}/CommonMacros.txt)

# Set paths for external libraries
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/External/imgui)
set(ImGuizmo_DIR ${CMAKE_SOURCE_DIR}/External/ImGuizmo)
set(NRI_DIR ${CMAKE_SOURCE_DIR}/External/_NRI_SDK)
set(NRD_DIR ${CMAKE_SOURCE_DIR}/External/_NRD_SDK)


# ImGui source files
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp 
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

# Add the libs subdirectory
add_subdirectory(External/glfw)
add_subdirectory(External/glm)
add_subdirectory(External/vk-bootstrap)
add_subdirectory(External/VulkanMemoryAllocator)
add_subdirectory(External/tracy)
add_subdirectory(External/fastgltf)
add_subdirectory(External/meshoptimizer)

# Find all source files
file(GLOB SOURCES "Source/*.cpp" "Source/*.c")

# Add Vulkan
find_package(Vulkan REQUIRED)

# Create the executable
add_executable(Spark ${SOURCES}
                              ${TRACY_SOURCES} 
                              ${IMGUI_SRC} 
                              ${IMGUI_BACKEND_SRC} 
                              ${ImGuizmo_SRC})

# Add include directories
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
    ${NRI_DIR}/Include
    ${NRD_DIR}/Integration
    ${NRD_DIR}/Include
)

target_compile_definitions(Spark PRIVATE TRACY_ENABLE)

# Link libraries
target_link_libraries(Spark PRIVATE 
    glfw 
    glm 
    vk-bootstrap 
    VulkanMemoryAllocator
    fastgltf
    meshoptimizer
    Vulkan::Vulkan
)

# Add NRD library with debug/release configuration
if(EXISTS ${NRD_DIR}/Lib)
    # Create imported target for NRD
    add_library(NRD STATIC IMPORTED)
    
    # Set the imported location based on build configuration
    set_target_properties(NRD PROPERTIES
        IMPORTED_LOCATION_DEBUG "${NRD_DIR}/Lib/Debug/NRD.lib"
        IMPORTED_LOCATION_RELEASE "${NRD_DIR}/Lib/Release/NRD.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${NRD_DIR}/Lib/Release/NRD.lib"
        IMPORTED_LOCATION_MINSIZEREL "${NRD_DIR}/Lib/Release/NRD.lib"
    )
    
    # Link against NRD
    target_link_libraries(Spark PRIVATE NRD)
    
    # Add include directory for NRD
    target_include_directories(Spark PRIVATE ${NRD_DIR}/Include)
endif()

# Add NRI library with debug/release configuration
if(EXISTS ${NRI_DIR}/Lib)
    # Create imported target for NRI
    add_library(NRI STATIC IMPORTED)
    
    # Set the imported location based on build configuration
    set_target_properties(NRI PROPERTIES
        IMPORTED_LOCATION_DEBUG "${NRI_DIR}/Lib/Debug/NRI.lib"
        IMPORTED_LOCATION_RELEASE "${NRI_DIR}/Lib/Release/NRI.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${NRI_DIR}/Lib/Release/NRI.lib"
        IMPORTED_LOCATION_MINSIZEREL "${NRI_DIR}/Lib/Release/NRI.lib"
    )
    
    # Link against NRI
    target_link_libraries(Spark PRIVATE NRI)
    
    # Add include directory for NRI
    target_include_directories(Spark PRIVATE ${NRI_DIR}/Include)
endif()


# Function to copy DLLs after build
function(copy_dll target dll_path)
    if(EXISTS ${dll_path})
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${dll_path}
                $<TARGET_FILE_DIR:${target}>
        )
    endif()
endfunction()

# Copy NRD DLLs
copy_dll(Spark "${NRD_DIR}/Lib/Debug/NRD.dll")
copy_dll(Spark "${NRD_DIR}/Lib/Release/NRD.dll")

# Copy NRI DLLs
copy_dll(Spark "${NRI_DIR}/Lib/Debug/NRI.dll")
copy_dll(Spark "${NRI_DIR}/Lib/Release/NRI.dll")