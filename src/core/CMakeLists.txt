# Placeholder empty library target
add_library(dupcleaner_core INTERFACE)
target_include_directories(dupcleaner_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
