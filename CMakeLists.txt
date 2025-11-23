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
)

# -----------------------------
# Your executable
# -----------------------------
add_executable(Bolt
    src/main.cpp
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
