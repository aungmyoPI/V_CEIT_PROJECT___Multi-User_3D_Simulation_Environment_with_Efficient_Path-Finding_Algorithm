# 🧪 V_Project — 3D Simulation & Interaction Framework

**High-performance C++17 + OpenGL 3.3 research platform** for multi-agent interaction, rendering experiments, and pathfinding/AI research. This repository contains the rendering engine, animation and AI systems, and tools used in the CEIT project.

---

## Summary

- Deferred shading pipeline, instanced rendering (very large vegetation counts), skeletal animation blending, and a custom grid-based A* pathfinding system optimized to avoid heap allocations.
- ImGui-based UI for login/registration and an optional asynchronous persistence layer to MongoDB referenced in the project (the persistence server/bridge is not included in this repository).

---

## ✨ Features

- Deferred Lighting Pipeline (G-buffer + lighting pass)
- Skeletal Animation System (animation blending, bone transforms)
- Grid-based A* Pathfinding with dynamic obstacle avoidance
- Instanced Rendering for high-density vegetation
- ImGui-powered developer UI
- Build integration via CMake (FetchContent for GLFW, Assimp, ImGui)

---

## 🛠️ Technical Stack

- Language: C++17
- Graphics API: OpenGL 3.3 Core Profile (GLAD)
- Windowing & Input: GLFW
- Math: GLM
- Model Import: Assimp
- UI: Dear ImGui
- Build: CMake 3.15+ and Ninja (recommended)
- License: MIT (see LICENSE)

---

## 📦 Prerequisites

- A C++17-capable compiler (MSVC / Visual Studio 2019+, GCC 9+, Clang 10+)
- CMake 3.15+
- Ninja (recommended)
- Git (for FetchContent in CMake)

---

## 🚀 Quick start (cross-platform)

Clone and build (single-line, works on Windows/macOS/Linux when Ninja is available):

```bash
git clone https://github.com/aungmyoPI/V_CEIT_PROJECT___Multi-User_3D_Simulation_Environment_with_Efficient_Path-Finding_Algorithm.git
cd V_CEIT_PROJECT___Multi-User_3D_Simulation_Environment_with_Efficient_Path-Finding_Algorithm
cmake -B build -G Ninja
ninja -C build

# Run (Linux / macOS)
./build/V_Project

# Run (Windows - PowerShell or CMD)
.\build\V_Project.exe
```

Notes:
- The CMake configuration uses FetchContent to download and build GLFW, Assimp, and Dear ImGui on first configure; the first build may take longer.
- The build target name is `V_Project` (not a packaged installer).

---

## State persistence / MongoDB

The README previously referenced an asynchronous MongoDB persistence bridge. At the time of this edit, there is no separate persistence server or Python bridge checked in under the repository root. The codebase contains hooks and references to state persistence; if you plan to use persistence:

- Provide a running MongoDB instance and the persistence bridge/service expected by your build (not included by default), or
- Run the application without the persistence backend (the application runs locally but without persistent server-backed save/load).

If you'd like, I can add a minimal example persistence bridge or document the exact runtime flags and environment variables once you provide the persistence script or confirm how you run it.

---

## Controls

- Movement: W / A / S / D
- Basic Attack: Left Mouse Button
- Skill Ability: E
- Ultimate Ability: Q
- Release mouse cursor: Left Alt
- Exit: Esc

---

## Project layout

```text
assets/          # models, textures, animations (large files may be kept externally)
include/         # third-party headers and project headers
shaders/         # GLSL shaders
src/             # project source files (renderer, game loop, AI, animation, etc.)
lib/             # optional external libraries
CMakeLists.txt   # build configuration
README.md        # this file
LICENSE          # MIT license
```

---

## Authors & Contributors

This project is academic in nature; it is best to be explicit about roles rather than listing percentage splits.

- Lead author
  - Aung Myo Pai — project lead, rendering, AI/pathfinding, and build system.
  - GitHub: https://github.com/aungmyoPI

- Major contributor
  - (Add name) — animation system, character assets, or other major contribution.

- Contributors
  - (Add teammate name) — networking, testing, scene setup, minor fixes.
  - (Add teammate name) — documentation, asset preparation, minor bug fixes.

- Supervisors / Academic Mentors
  - (Add supervisor name) — project supervision and guidance.

Acknowledgements
- Thanks to the maintainers of third-party libraries used in this project: GLFW, GLM, Assimp, Dear ImGui, stb.

Notes on contributor policy
- For a professional repository, prefer named roles (Lead author, Major contributor, Contributors, Supervisors) over precise percentage values in the README. If you want an exact contribution breakdown, create a CONTRIBUTORS.md with commit-based metrics or a private record for assessment purposes.

---

## Contributing

If you want to accept contributions or have teammates update the repo, consider adding CONTRIBUTING.md and a short CODE_OF_CONDUCT. I can add templates for those if you want.

---

## Screenshot / Demo

Add a short GIF or screenshot in `assets/` and reference it here. Example:

![screenshot](assets/screenshot.png)

(If you provide a screenshot I will add it to the README in place of this placeholder.)

---

## License

MIT — see LICENSE file for details.

---

If you want, I will:
- Replace the placeholder contributor names with the real names and roles (if you give them), and
- Add a CONTRIBUTORS.md with a computed commit summary (git shortlog) and move any minor names to Acknowledgements, or
- Create a small persistence example (Python + pymongo) and document how to configure/run it.

Tell me which of these you'd like next and I will make the change in a follow-up commit.