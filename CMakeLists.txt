cmake_minimum_required(VERSION 3.21)
project(Bolt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Dependencies
# -----------------------------
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Magnum core + graphics
find_package(Magnum CONFIG REQUIRED
    GL
    Primitives
    Shaders
    Trade
    WglContext
    SceneGraph
)

# -----------------------------
# Your executable
# -----------------------------
add_executable(Bolt
    src/main.cpp
    src/Bolt.h
)

# Path to your assets
set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/res")            # C:\Git\Bolt\res
set(ASSETS_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/res")    # C:\Git\Bolt\build\ninja-debug\res

# Copy res/ => build/res/ on every build
add_custom_command(
    TARGET Bolt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ASSETS_SOURCE_DIR}"
            "${ASSETS_TARGET_DIR}"
)

# -----------------------------
# Linking
# -----------------------------
target_link_libraries(Bolt PRIVATE
    SDL3::SDL3
    imgui::imgui

    # Magnum core libs
    Magnum::GL
    Magnum::Trade
    Magnum::Primitives
    Magnum::Shaders
    Magnum::GLContext

    Magnum::SceneGraph
)

# -----------------------------
# Platform notes
# -----------------------------
# On Windows, ensure the app runs next to DLLs
#if(WIN32)
#    add_custom_command(TARGET Bolt POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#            "${CURRENT_INSTALLED_DIR}/bin"
#            "$<TARGET_FILE_DIR:Bolt>"
#    )
#endif()
