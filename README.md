# V_Project

**V_Project** is a 3D Action RPG prototype built using C++17, OpenGL 3.3, and CMake. Featuring deferred rendering, skeletal mesh animation, dynamic pathfinding, instanced foliage, and an integrated Dear ImGui authentication UI, this project serves as a testbed for modern rendering techniques and gameplay logic.

---

## Features

* **Deferred Lighting Pipeline:** G-buffer rendering pass processing world positions, normals, albedo, and specular parameters alongside dynamic point light support.
* **Skeletal Animation System:** Real-time character skeletal animation blending for idle, running, attacking, and special abilities.
* **A\* Pathfinding AI:** Grid-based A\* navigation enabling aggressive mutant AI to track players around forest obstacles dynamically.
* **Instanced Rendering:** High-density vegetation rendering capable of drawing millions of grass blades and forest trees efficiently.
* **Integrated Authentication & State Management:** ImGui-powered login and registration windows with asynchronous background state persistence.

---

## Technical Stack

| Component | Technology / Library |
| :--- | :--- |
| **Language** | C++17 |
| **Graphics API** | OpenGL 3.3 Core Profile (GLAD loader) |
| **Windowing & Input** | GLFW 3.3.8 |
| **Mathematics** | GLM (OpenGL Mathematics) |
| **Model Import** | Assimp v5.2.5 |
| **UI Framework** | Dear ImGui v1.91.0 |
| **Build System** | CMake (3.15+) & Ninja |

---

## Prerequisites

Ensure you have the following installed on your system before building:

* **Compiler:** MSVC (Visual Studio 2019+), GCC 9+, or Clang 10+ with C++17 support.
* **CMake:** Version 3.15 or higher.
* **Ninja Build System:** Recommended for fast, parallel builds.
* **Git:** Required for CMake `FetchContent` to download dependencies automatically (GLFW, Assimp, ImGui).

---

## Quick Start (Building & Running)

Clone the repository and run the single build command:

```bash
cmake -B build -G Ninja && ninja -C build && ./build/V_Project.exe

```

> **Note:** The initial CMake configuration downloads external dependencies via `FetchContent` (e.g., Assimp, GLFW, ImGui). Subsequent builds will be significantly faster.
> 
> 

---

## Controls

* **Movement:** `W` / `A` / `S` / `D`

* **Basic Attack:** `Left Mouse Button`

* **Skill Ability:** `E`

* **Ultimate Ability:** `Q`

* **Release Mouse Cursor:** Hold `Left Alt`

* **Exit Game:** `Esc`
# V_CEIT_PROJECT___Multi-User_3D_Simulation_Environment_with_Efficient_Path-Finding_Algorithm
