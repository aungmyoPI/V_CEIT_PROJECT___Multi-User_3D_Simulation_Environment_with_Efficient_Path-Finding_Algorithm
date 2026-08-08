# V_CEIT_PROJECT: Multi-User 3D Simulation Environment with Efficient Path-Finding Algorithm

[![License: MIT](https://shields.io)](https://opensource.org)
[![Language](https://shields.io)](https://cppreference.com)
[![Graphics API](https://shields.io)](https://opengl.org)

## The Pitch
V_Project is a custom C++17/OpenGL 3.3 3D Action RPG prototype designed to test performance boundaries. It focuses on high-density instanced rendering, deferred shading, and complex AI navigation without relying on heavy game engines.

---

## Media & Visuals
*Add an animated GIF or a link to a YouTube video showcasing your character moving, your instanced grass blowing, and the pathfinding AI chasing the player!*

---

## Architectural Highlights

*   **⚡ Deferred Shading:** Two-pass pipeline separating geometric data (G-Buffer) from lighting calculations for optimized rendering.
*   **🌲 Instanced Rendering:** Efficiently draws millions of grass blades and trees using GPU-packed transformation matrices.
*   **🧠 A* Pathfinding AI:** Dynamic grid-based navigation system for enemy chasing behavior.

---

## Technical Stack
Built with Modern C++17, OpenGL 3.3 Core, GLFW 3.3.8, GLM, Assimp, Dear ImGui, and CMake.

---

## Quick Start (Building & Running)
1. **Prerequisites:** C++17 Compiler (MSVC/GCC/Clang), CMake (3.15+), Ninja Build.
2. **Build & Run:**
   ```bash
   cmake -B build -G Ninja && ninja -C build && ./build/V_Project.exe
   ```

---

## Game Controls
- **Movement:** `W` / `A` / `S` / `D`
- **Attack:** `Left Mouse` / `E` / `Q`
- **Unlock Cursor:** `Left Alt`
- **Exit:** `Esc`

---

## License
Distributed under the permissive MIT License. See `LICENSE` for more information.
