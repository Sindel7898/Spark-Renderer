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
![GI Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/2d39aed7d3a47194528df01d15a12ed505914e8a/GitHub%20Doc/Screenshot%202026-04-21%20135936.png)

![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/2d39aed7d3a47194528df01d15a12ed505914e8a/GitHub%20Doc/Screenshot%202026-04-21%20140125.png)


![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/2d39aed7d3a47194528df01d15a12ed505914e8a/GitHub%20Doc/Screenshot%202026-04-21%20135819.png)


![Spark Renderer Screenshot](https://github.com/Sindel7898/Spark-Renderer/blob/2d39aed7d3a47194528df01d15a12ed505914e8a/GitHub%20Doc/Screenshot%202026-04-21%20135700.png)



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

