# Spark Renderer

Spark is an experimental 3D renderer built with Vulkan, aimed at finding a balance between real-time performance and high-quality rendering.
It serves as a test bed for experimenting with modern rendering techniques.

### Purpose
Spark is a renderer and testbed for prototyping and implementing new graphics techniques without rebuilding core systems from scratch.

## Core Features
- **Mix of Raytraced and Rasterised Rendering**
- **Reservoir-based SpatioTemporal Importance Resampling -  Direct Illumination (ReSTIR DI)** for efficient ray-traced shadows

### Global Illumination
- **Probe-based Dynamic Diffuse Global Illumination (DDGI)** — real-time indirect lighting via raytraced irradiance probes
- **DDGI Resampling** — Efficient blending of ReSTIR DI and DDGI for more accurate indirect illumination

### Neural Rendering Features
- **Nvidia Ray Reconstruction** — Real-time Neueal Denoiser
  
### Screen-Space Effects
- Screen-space ambient occlusion (SSAO)
- Screen-space global illumination (SSGI)

### Material System
- Physically-based rendering (PBR) workflow
- Metallic-roughness material model

## Media

*(Development previews - more coming soon)*  
![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/6d9723fd48756ba83af466cae59fa1753bcfb0ba/GitHub%20Doc/Screenshot%202025-10-11%20013658.png)


![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/6d9723fd48756ba83af466cae59fa1753bcfb0ba/GitHub%20Doc/Screenshot%202025-09-10%20204900.png)


![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/6d9723fd48756ba83af466cae59fa1753bcfb0ba/GitHub%20Doc/G28XgLNXUAAWW4P.jpg)


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
cmake ..

