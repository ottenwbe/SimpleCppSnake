# CPP Simple Snake

[![C++ CI](https://github.com/ottenwbe/SimpleCppSnake/actions/workflows/ci.yml/badge.svg)](https://github.com/ottenwbe/SimpleCppSnake/actions/workflows/ci.yml)

<!-- Note about AI assistance -->
> **Note:** This project was created with assistance of AI (Gemini Code Assist), with the purpose of testing some capabilites of AI assisted software development.

A CPP Simple Snake game implementation using modern C++17, OpenGL (Core Profile), GLFW, and FreeType for text rendering.

**Note: This project is specifically targeted for Linux.** It utilizes Linux-specific APIs (such as `/proc/self/exe`) for path resolution and is optimized for Linux-based build environments.

## Features

*   **Smooth Gameplay**: Classic snake mechanics with an increasing difficulty curve.
*   **Special Collectibles**: 
    *   **Banana**: Worth +2 points, spawns periodically.
    *   **Golden Apple**: Rare +5 point bonus.
    *   **Visual Timers**: Special items feature shrinking progress bars indicating remaining time.
*   **AI Auto-Pilot**: Built-in BFS-based pathfinding to help the snake find food automatically.
*   **Obstacles**: Procedural obstacle generation to increase challenge.
*   **High Score System**: Persistent Top 5 high score tracking (saved to `highscore.txt`).
*   **Modern Graphics**: Procedural shaders for apples, bananas, and a beveled playfield.

## Prerequisites

To build this project, you need the following tools installed on your system:

*   **C++ Compiler**: GCC, Clang, or MSVC (supporting C++17).
*   **CMake**: Version 3.15 or higher.
*   **Conan**: The C/C++ Package Manager (Install via `pip install conan`).
*   **System Headers**: Ensure OpenGL and X11 development headers are installed (e.g., `libgl1-mesa-dev`, `libx11-dev` on Ubuntu).

## Dependencies & Attributions

This project is built using several excellent open-source libraries managed via Conan:

*   **[GLFW](https://github.com/glfw/glfw)** (Zlib License): Window management and input.
*   **[GLM](https://github.com/g-truc/glm)** (MIT License): Mathematics library for graphics.
*   **[FreeType](https://www.freetype.org)** (FTL License): Font rendering. *Portions of this software are copyright © 2026 The FreeType Project. All rights reserved.*
*   **[spdlog](https://github.com/gabime/spdlog)** (MIT License): Thread-safe logging.
*   **[Catch2](https://github.com/catchorg/Catch2)** (Boost Software License 1.0): C++ Unit Testing Framework.
*   **OpenGL**: Hardware-accelerated rendering.
*   **Fonts**: System fonts (DejaVu/OpenSans) are subject to their respective licenses.

## Build Instructions

### 1. Setup Conan Profile (First time only)

If you haven't used Conan before, create a default profile:

```bash
conan profile detect --force
```

### 2. Install Dependencies

Download and configure the libraries using Conan. This will generate the necessary CMake toolchain files in the `build` directory.

```bash
conan install . --output-folder=build --build=missing
```

### 3. Build with CMake

Now, simply run the build script from the project's root directory:

```bash
./build.sh
```

## Running the Game

After building, the executable will be located in the `build` directory.

```bash
./CPPSimpleSnake
```

## Controls

*   **Arrow Keys**: Move Snake / Navigate Menu
*   **Space / Enter**: Select / Pause Game
*   **Esc**: Go Back / Quit
*   **F11**: Toggle Fullscreen
*   **I**: Toggle AI Auto-Pilot

## License and Attributions

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.