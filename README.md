# Pacman-OpenGL

Minimal cross-platform OpenGL starter project using CMake, GLFW and GLAD.

## Prerequisites
- CMake 3.20+
- A C++17 compiler (Visual Studio 2022 / clang / gcc)
- Git

On macOS you will also need the Xcode command line tools. On Windows install the [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/) or full Visual Studio with the "Desktop development with C++" workload.

## Configure & Build

```bash
cmake -S . -B build
cmake --build build
```

The executable `PacmanOpenGL` (or `PacmanOpenGL.exe` on Windows) and shader assets are placed in the build directory.

## Run

```bash
./build/PacmanOpenGL   # macOS / Linux
build\PacmanOpenGL.exe # Windows
```

You should see a window displaying a colorful triangle that exercises the shader pipeline.

## Project Structure

```
Pacman-OpenGL/
├── CMakeLists.txt            # คอนฟิกบิลด์หลัก
├── README.md                 # เอกสารรวม
├── compile_commands.json     # ดัชนีให้ clangd/LSP
├── external/                 # ส่วนหัว third-party (cgltf, glad)
├── pac_man_map_moderno/      # asset ของกระดาน/ตัวเลข
├── shaders/                  # GLSL assets (simple.vert/frag)
├── src/
│   ├── core/                 # types, กล้อง, หน้าต่าง
│   ├── utils/                # ฟังก์ชันช่วยเหลือ (ไฟล์, bounds)
│   ├── rendering/            # shader, mesh, primitives
│   ├── game/
│   │   ├── map/              # board, map generator
│   │   └── player/           # state/การเคลื่อนไหวผู้เล่น
│   └── main.cpp              # entry point (init + loop)
└── build*/                   # โฟลเดอร์ที่ CMake สร้าง (อย่าแก้)
```