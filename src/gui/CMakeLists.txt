include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.8-docking # Use docking branch as it's generally preferred
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})

add_executable(dupcleaner_gui 
    main.cpp
    app.cpp
    formatting.cpp
    thumbnail_logic.cpp
    thumbnail_cache.cpp
    selection_logic.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(dupcleaner_gui PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(dupcleaner_gui PRIVATE dupcleaner_core glfw)

if (WIN32)
    target_link_libraries(dupcleaner_gui PRIVATE opengl32)
elseif (APPLE)
    target_link_libraries(dupcleaner_gui PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(dupcleaner_gui PRIVATE OpenGL::GL)
endif()
