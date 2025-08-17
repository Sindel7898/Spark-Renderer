# Spark Renderer 🔥

**Spark** is an experimental 3D renderer in heavy development, designed to bridge the gap between **real-time performance** and **high-quality rendering**. Built with games in mind, it combines modern graphics techniques while prioritizing flexibility for future innovation.

*⚠️ Note: This project is under active development - APIs may change and features are being rapidly iterated on.*


![Renderer Screenshot]([GitHub Doc/GI.png](https://github.com/Sindel7898/Spark-Renderer/blob/main/GitHub%20Doc/GI.png))

## ✨ Core Features

### Hybrid Rendering Pipeline
- **Ray tracing** for accurate lighting and reflections
- **Rasterization** for high-performance rendering
- Adaptive quality system that balances visual fidelity and frame rate

### Screen-Space Effects
- Screen-space reflections (SSR)
- Screen-space ambient occlusion (SSAO)
- Screen-space global illumination (SSGI)

### Material System
- Physically-based rendering (PBR) workflow
- Metallic-roughness material model
- HDR environment lighting

## 📸 Media

*(Development previews - more coming soon)*  
![Scene 1](media/scene1.jpg) *Early hybrid rendering test*  
![Scene 2](media/scene2.jpg) *PBR materials demonstration*

## 🚀 Getting Started (Developers)

*Early access for contributors - expect breaking changes*

### Prerequisites
- CMake 3.20+
- Vulkan SDK (1.3+)
- C++20 compatible compiler

### Building
```bash
git clone https://github.com/yourusername/spark-renderer.git
cd spark-renderer
mkdir build && cd build
cmake ..
make -j8
