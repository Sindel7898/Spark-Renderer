C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\SkyBox_Shader.vert         -o  Shaders\Compiled_Shader_Files\SkyBox_Shader.vert.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\Light_Shader.vert          -o  Shaders\Compiled_Shader_Files\Light_Shader.vert.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\GeometryPass.vert          -o  Shaders\Compiled_Shader_Files\GeometryPass.vert.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\FullScreenQuad.vert        -o  Shaders\Compiled_Shader_Files\FullScreenQuad.vert.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\Terrain_GeometryPass.vert  -o  Shaders\Compiled_Shader_Files\Terrain_GeometryPass.vert.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\Grass_GeoPass.vert         -o  Shaders\Compiled_Shader_Files\Grass_GeoPass.vert.spv


C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\SkyBox_Shader.frag         -o  Shaders\Compiled_Shader_Files\SkyBox_Shader.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\Light_Shader.frag          -o  Shaders\Compiled_Shader_Files\Light_Shader.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\GeometryPass.frag          -o  Shaders\Compiled_Shader_Files\GeometryPass.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\DefferedLightingPass.frag  -o  Shaders\Compiled_Shader_Files\DefferedLightingPass.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\SSAO_Shader.frag           -o  Shaders\Compiled_Shader_Files\SSAO_Shader.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\SSAOBlur_Shader.frag       -o  Shaders\Compiled_Shader_Files\SSAOBlur_Shader.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\FXAA.frag                  -o  Shaders\Compiled_Shader_Files\FXAA.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\Terrain_GeometryPass.frag  -o  Shaders\Compiled_Shader_Files\Terrain_GeometryPass.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\Grass.frag                 -o  Shaders\Compiled_Shader_Files\Grass.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\SSR.frag                   -o  Shaders\Compiled_Shader_Files\SSR.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\CombinedImage.frag         -o  Shaders\Compiled_Shader_Files\CombinedImage.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\SSGI.frag                  -o  Shaders\Compiled_Shader_Files\SSGI.frag.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe Shaders\Shader_Files\BilateralFilter.frag                  -o  Shaders\Compiled_Shader_Files\BilateralFilter.frag.spv

glslc.exe Shaders/Shader_Files/raygen.rgen          -o Shaders/Compiled_Shader_Files/raygen.rgen.spv          --target-env=vulkan1.4
glslc.exe Shaders/Shader_Files/closesthit.rchit     -o Shaders/Compiled_Shader_Files/closesthit.rchit.spv     --target-env=vulkan1.4
glslc.exe Shaders/Shader_Files/ShadowMiss.rmiss     -o Shaders/Compiled_Shader_Files/ShadowMiss.rmiss.spv     --target-env=vulkan1.4
glslc.exe Shaders/Shader_Files/RayGenMiss.rmiss     -o Shaders/Compiled_Shader_Files/RayGenMiss.rmiss.spv     --target-env=vulkan1.4



pause

