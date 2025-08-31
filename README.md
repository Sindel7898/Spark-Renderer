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
![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/69d2235af25262cd5fdce4980d19623217c82c05/GitHub%20Doc/Screenshot%202025-08-23%20151346.png)
![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/e81f4d323526f9d871e7800e8e0ce082db4dd0d4/GitHub%20Doc/Screenshot%202025-08-31%20203030.png)

![GI Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/69d2235af25262cd5fdce4980d19623217c82c05/GitHub%20Doc/Screenshot%202025-08-23%20010645.png)


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
