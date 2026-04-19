cmake_minimum_required(VERSION 3.15)
project(SimpleCppSnake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- GLFW ---
# Conan provides 'glfw3', system installs might provide 'glfw3' or 'GLFW'
find_package(glfw3 REQUIRED)

# --- GLM ---
# Conan uses 'glm', system installs might use 'glm'
find_package(glm REQUIRED)

# --- OpenGL ---
find_package(OpenGL REQUIRED)

# --- Freetype ---
find_package(Freetype REQUIRED)

# --- spdlog ---
find_package(spdlog REQUIRED)

# --- Catch2 ---
find_package(Catch2 REQUIRED)

# --- Find a system font ---
find_file(SYSTEM_FONT_PATH
    NAMES
       "DejaVuSans.ttf" # Very common on Ubuntu/Debian
       "LiberationSans-Regular.ttf" # Another common open-source font
       "OpenSans-Light.ttf" # Original choice
       "NotoSans-Regular.ttf" # Google's Noto font, often available
    PATHS
        /usr/share/fonts/dejavu-sans-fonts
        /usr/share/fonts/open-sans
        /usr/share/fonts/liberation-sans
        /usr/share/fonts/droid-sans-fonts
        /usr/share/fonts/truetype/dejavu
        /usr/share/fonts/truetype/liberation
        /usr/share/fonts/truetype/noto
        /usr/share/fonts/TTF
        /usr/share/fonts
        /usr/local/share/fonts # Broader search path
)

# --- Executable ---
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/game_logic.cpp
    src/renderer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

# --- Enable Testing ---
enable_testing()

# --- Compiler Options ---
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

# Link against modern namespaced targets. 
# Conan's CMakeDeps generator creates these targets (glfw::glfw, etc).
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw
    OpenGL::GL 
    glm::glm 
    Freetype::Freetype
    spdlog::spdlog
)

# --- Unit Tests ---
add_executable(unit_tests tests/test_main.cpp src/game_logic.cpp)
target_include_directories(unit_tests PRIVATE include)

target_link_libraries(unit_tests PRIVATE 
    Catch2::Catch2WithMain
    glm::glm 
    spdlog::spdlog
)

if(NOT SYSTEM_FONT_PATH)
    message(FATAL_ERROR "Could not find a suitable TTF font. Please install a common font like 'dejavu-sans-fonts' or 'liberation-fonts'.")
else()
    message(STATUS "Using system font: ${SYSTEM_FONT_PATH}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE FONT_PATH="${SYSTEM_FONT_PATH}")
endif()

add_test(NAME GameLogicTests COMMAND unit_tests)
