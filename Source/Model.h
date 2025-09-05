#pragma once

#include <memory>
#include <string>
#include "VulkanContext.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Light.h"
#include "Drawable.h"
#include "structs.h"

struct PushConstantData {
   glm::mat4 worldMatrix;
   glm::vec4 MaterialIndex_Padding;
};



struct VertexData {
    alignas(16) glm::mat4 ViewMatrix;
    alignas(16) glm::mat4 ProjectionMatrix;
};

struct GPU_InstanceData {
    glm::vec4 bCubeMapReflection_bScreenSpaceReflectionWithPadding;
};

struct InstanceData {
    bool bCubeMapReflection = true;
    bool bScreenSpaceReflection = true;

    std::shared_ptr<GPU_InstanceData> gpu_InstanceData  = std::make_shared<GPU_InstanceData>();
    VulkanContext* vulkanContext = nullptr;

    InstanceData(InstanceData* LastInstanceData,VulkanContext* vulkanRef)
    {
        vulkanContext = vulkanRef;

        if (LastInstanceData)
        {
            glm::vec3 PositionOffset = glm::vec3(2, 0, 0);

            Position = LastInstanceData->Position + PositionOffset;
            Scale = LastInstanceData->Scale;
            Rotation = LastInstanceData->Rotation;

            CubeMapReflectiveSwitch(LastInstanceData->bCubeMapReflection);
            ScreenSpaceReflectiveSwitch(LastInstanceData->bScreenSpaceReflection);
        }
        
        UpdateTrasnformationMatrix();
        UpdateGPU_ReflectionFlags();
    }

    glm::vec3 GetPostion () const { return Position;}
    glm::vec3 GetRotation() const { return Rotation;}
    glm::vec3 GetScale   () const { return Scale; }
    glm::mat4 GetTransformationMatrix() const { return TransformationMatrix; }
    glm::mat4 GetModelMatrix() const { return ModelMatrix; }

    void CubeMapReflectiveSwitch(bool breflective)
    {
        bCubeMapReflection = breflective;
        UpdateGPU_ReflectionFlags();
    }

    void ScreenSpaceReflectiveSwitch(bool breflective)
    {
        bScreenSpaceReflection = breflective;
        UpdateGPU_ReflectionFlags();
    }
    void UpdateGPU_ReflectionFlags() {
        gpu_InstanceData->bCubeMapReflection_bScreenSpaceReflectionWithPadding = glm::vec4(bCubeMapReflection, bScreenSpaceReflection, 0, 0);
     }
 
    void SetPostion(glm::vec3 NewPosition)
    {
        Position = NewPosition;
        UpdateTrasnformationMatrix();
    }

    void SetRotation(glm::vec3 NewRotation)
    {
        Rotation = NewRotation;
        UpdateTrasnformationMatrix();
    }

    void SetScale(glm::vec3 NewScale)
    {
        Scale = NewScale;
        UpdateTrasnformationMatrix();
    }

    void SetTrasnformationMatrix(glm::mat4 NewTrasnformationlMatrix)
    {
        TransformationMatrix = NewTrasnformationlMatrix;
        BreakDownAndUpdateModelMatrix();
    }

    void SetModelMatrix(glm::mat4 NewModelMatrix)
    {
        ModelMatrix = NewModelMatrix;
    }

    void UpdateTrasnformationMatrix()
    {
        TransformationMatrix = glm::mat4(1.0f);
        TransformationMatrix = glm::translate(TransformationMatrix, Position);
        TransformationMatrix = glm::rotate(TransformationMatrix, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        TransformationMatrix = glm::rotate(TransformationMatrix, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        TransformationMatrix = glm::rotate(TransformationMatrix, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        TransformationMatrix = glm::scale(TransformationMatrix, Scale);

        vulkanContext->ResetTemporalAccumilation();
    }

    void BreakDownAndUpdateModelMatrix()
    {
        glm::vec3 Newscale;
        glm::quat Newrotation;
        glm::vec3 Newtranslation;
        glm::vec3 Newskew;
        glm::vec4 Newperspective;

        // Decompose the matrix
        glm::decompose(TransformationMatrix, Newscale, Newrotation, Newtranslation, Newskew, Newperspective);

        Position = Newtranslation;

        Rotation = glm::degrees(glm::eulerAngles(Newrotation));

        Scale = Newscale;
        vulkanContext->ResetTemporalAccumilation();
    }

    private:

    glm::vec3 Position  = glm::vec3(1);
    glm::vec3 Scale     = glm::vec3(1);
    glm::vec3 Rotation  = glm::vec3(1);
    glm::mat4 TransformationMatrix = glm::mat4(1);
    glm::mat4 ModelMatrix = glm::mat4(1);
};

class Model : public Drawable
{
public:

    Model(const std::string filepath, VulkanContext* vulkancontext, vk::CommandPool commandpool, Camera* rcamera, BufferManager* buffermanger);
    void LoadTextures();
    void CreateVertexAndIndexBuffer() override;
    void createDescriptorSetLayout() override;
    void createDescriptorSets(vk::DescriptorPool descriptorpool) override;
    void CreateUniformBuffer() override;
    void Instantiate();
    void Destroy(int instanceIndex);
    void UpdateUniformBuffer(uint32_t currentImage);
    bool CalcDistasnceCulling(glm::mat4 Matrix);
    void DrawNode(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, uint32_t imageIndex, const std::vector<std::shared_ptr<Node>>& nodes, const glm::mat4& parentMatrix);
    void Draw(vk::CommandBuffer commandbuffer, vk::PipelineLayout  pipelinelayout, uint32_t imageIndex) override;
    void CreateBLAS();

    void CleanUp() ;

    vk::DescriptorSetLayout  RayTracingDescriptorSetLayout;

    std::vector<ImageData> AlbedoTextures;
    std::vector<ImageData> NormalTextures;
    std::vector<ImageData> MetallicRoughnessTextures;
    std::vector<ImageData> AOTextures;


    vk::AccelerationStructureKHR BLAS;

    std::vector<std::unique_ptr<InstanceData>>     Instances;
    std::vector<std::shared_ptr<GPU_InstanceData>> GPU_InstancesData;

    std::vector<BufferData> Model_GPU_DataUniformBuffers;
    std::vector<void*>      Model_GPU_DataUniformBuffersMappedMem;

    size_t materialCount;

    std::vector<std::vector<vk::DescriptorSet>> SceneDescriptorSets;

private:


    std::string FilePath;
    const StoredModelData* storedModelData = nullptr;
    BufferData BLAS_Buffer;
    BufferData BLAS_ScratchBuffer;

};


static inline void ModelDeleter(Model* model) {

        if (model) {
            model->CleanUp();
            delete model;
        }
   
};