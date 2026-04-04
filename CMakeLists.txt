cmake_minimum_required(VERSION 3.15)
project(SimpleCppSnake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- GLFW ---
# Conan provides 'glfw3', system installs might provide 'glfw3' or 'GLFW'
find_package(glfw3 3.3 REQUIRED)

# --- GLM ---
# Conan uses 'glm', system installs might use 'glm'
find_package(glm REQUIRED)

# --- OpenGL ---
find_package(OpenGL REQUIRED)

# --- Freetype ---
find_package(Freetype REQUIRED)

# --- spdlog ---
find_package(spdlog REQUIRED)

# --- Find a system font ---
find_file(SYSTEM_FONT_PATH
    NAMES
       "OpenSans-Light.ttf"
    PATHS
        /usr/share/fonts/dejavu-sans-fonts
        /usr/share/fonts/open-sans
        /usr/share/fonts/liberation-sans
        /usr/share/fonts/droid-sans-fonts
        /usr/share/fonts/truetype/dejavu
        /usr/share/fonts/truetype/liberation
        /usr/share/fonts/TTF
        /usr/share/fonts
)

# --- Executable ---
add_executable(${PROJECT_NAME} main.cpp)

# Link against modern namespaced targets. 
# Conan's CMakeDeps generator creates these targets (glfw::glfw, etc).
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw 
    OpenGL::GL 
    glm::glm 
    Freetype::Freetype
    spdlog::spdlog
)

if(NOT SYSTEM_FONT_PATH)
    message(FATAL_ERROR "Could not find a suitable TTF font. Please install a common font like 'dejavu-sans-fonts' or 'liberation-fonts'.")
else()
    message(STATUS "Using system font: ${SYSTEM_FONT_PATH}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE FONT_PATH="${SYSTEM_FONT_PATH}")
endif()
