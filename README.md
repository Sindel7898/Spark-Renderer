# Spark Renderer

**Spark** is an experimental 3D renderer built with **Vulkan**, aimed at finding a balance between **real-time performance** and **high-quality rendering**.  
It serves as a **Test Bed** for experimenting with modern rendering techniques.  

### Purpose
Spark is a renderer And **testbed** for prototype and implement new graphics techniques without rebuilding core systems from scratch.  

## Core Features

### Hybrid Rendering Pipeline
- **Ray tracing** for accurate Shadows
- **Rasterization** for high-performance rendering
- Adaptive quality system that balances visual fidelity and frame rate

### Screen-Space Effects
- Screen-space reflections (SSR)
- Screen-space ambient occlusion (SSAO)
- Screen-space global illumination (SSGI)

### Material System
- Physically-based rendering (PBR) workflow
- Metallic-roughness material model

## Media

*(Development previews - more coming soon)*  
![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/dd422d41c6359996a35a17531d0eea607e59cab5/GitHub%20Doc/GI.png)

## Getting Started (Developers)

*Early access - expect breaking changes*

### Prerequisites
- CMake 3.20+
- Vulkan SDK (1.3+)
- C++20 compatible compiler

### Building
```bash
git clone https://github.com/Sindel7898/Spark-Renderer.git
cd Spark-Renderer
mkdir build && cd build
cmake ..
make -j8
