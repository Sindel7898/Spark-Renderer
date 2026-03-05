file(GLOB BASE_SRC "*.cpp" "*.hpp" "*.h")
list(APPEND BASE_SRC 
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/imgui.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/imgui_draw.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/imgui_tables.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/imgui_widgets.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
file(GLOB BASE_HEADERS "*.hpp" "*.h")

set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c)

add_library(base STATIC ${BASE_SRC} ${KTX_SOURCES})
if(NvPerf_FOUND)
    target_compile_definitions(base PRIVATE NV_PERF_ENABLE_INSTRUMENTATION)
endif()
if(WIN32)
    target_link_libraries(base ${Vulkan_LIBRARY} ${WINLIBS})
 else(WIN32)
    target_link_libraries(base ${Vulkan_LIBRARY} ${XCB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif(WIN32)

target_include_directories(base PUBLIC ${NvPerfUtility_IMPORTS_IMGUI_DIR})
