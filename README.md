# 🧪 3D Simulation & Interaction Framework

**A high-performance, custom-built 3D simulation framework** written in C++17 and OpenGL 3.3 Core Profile. Developed as a research platform for multi-agent interaction, it features a fully custom **deferred shading pipeline**, **instanced rendering** (1M+ grass blades with distance culling), **skeletal animation** blending, **A\* pathfinding** with static-memory optimization, and an **asynchronous client-server save system** using Winsock2 and MongoDB.

> **Note:** While it utilizes game-like assets (RPG characters, trees), this project is fundamentally a **graphics and AI engineering testbed** focused on rendering optimization, real-time pathfinding, and network persistence.

---

## ✨ Features

- **Deferred Lighting Pipeline** – G-buffer rendering pass processing world positions, normals, albedo, and specular parameters alongside dynamic point light support.
- **Skeletal Animation System** – Real-time character skeletal animation blending for idle, running, attacking, and special abilities.
- **A\* Pathfinding AI** – Grid-based A* navigation enabling aggressive AI agents to track players around forest obstacles dynamically.
- **Instanced Rendering** – High-density vegetation rendering capable of drawing millions of grass blades and forest trees efficiently.
- **Integrated Authentication & State Persistence** – ImGui-powered login and registration windows with asynchronous background state persistence to MongoDB.

---

## 🛠️ Technical Stack

| Component | Technology / Library |
|-----------|----------------------|
| Language | C++17 |
| Graphics API | OpenGL 3.3 Core Profile (GLAD loader) |
| Windowing & Input | GLFW 3.3.8 |
| Mathematics | GLM (OpenGL Mathematics) |
| Model Import | Assimp v5.2.5 |
| UI Framework | Dear ImGui v1.91.0 |
| Networking | Winsock2 (TCP/IP) |
| Database | MongoDB (via Python bridge) |
| Build System | CMake (3.15+) & Ninja |

---

## 📦 Prerequisites

Ensure you have the following installed before building:

- **Compiler**: MSVC (Visual Studio 2019+), GCC 9+, or Clang 10+ with C++17 support.
- **CMake**: Version 3.15 or higher.
- **Ninja Build System**: Recommended for fast, parallel builds.
- **Git**: Required for CMake FetchContent to download dependencies automatically.

---

## 🚀 Quick Start

Clone the repository and run:

```bash
cmake -B build -G Ninja
ninja -C build
./build/V_Project.exe
```

> **Note:** The initial CMake configuration downloads external dependencies via `FetchContent` (e.g., Assimp, GLFW, and ImGui). Subsequent builds will be significantly faster.

---

## 🎮 Controls

| Action | Key |
|--------|-----|
| Movement | W / A / S / D |
| Basic Attack | Left Mouse Button |
| Skill Ability | E |
| Ultimate Ability | Q |
| Release Mouse Cursor | Left Alt |
| Exit Game | Esc |

---

> **Note:** You can use dev acc for easy access to the project. [username : dev, password : dev]

---

## 📁 Project Structure

```text
assets/          # 3D models, textures, animations
include/         # Header files
shaders/         # GLSL vertex/fragment shaders
src/             # Source files
lib/             # External libraries
CMakeLists.txt   # Build configuration
README.md        # Project documentation
```

---

## 📄 License

This project is licensed under the MIT License – see the `LICENSE` file for details.

---

## 👥 Project Team

| Name | Role | Primary Contribution |
|------|------|----------------------|
| **Mg Aung Myo Pai** | **Lead Developer & System Integrator** | Core Engine, Deferred Rendering Pipeline, Instancing, Skeletal Animation, System Integration, Cross-module Debugging |
| Ma Khin Yadanar Win | Developer | Networking & Database Module (TCP Server, MongoDB Integration) |
| Mg Hlwan Moe Aung | Developer | A* Pathfinding Algorithm Development |
| Ma Thoon Thiri Swe | Developer | Finite State Machine (FSM) Logic Design |
| Mg Nay Phone Myint | *Contributor* | Documentation & Testing Support |
| Mg Kaung Khant Ko Ko | *Contributor* | Documentation & Testing Support |

**Supervised by:** **Dr. Thandar Soe, Professor**, Department of Computer Engineering and Information Technology, Technological University (Mandalay)

---

*Built as part of the CEIT Project at Technological University (Mandalay).*
