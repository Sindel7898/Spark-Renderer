#pragma once
#include <memory>
#include "VulkanContext.h"
#include <glm/fwd.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>


struct InstanceData {

    VulkanContext* vulkanContext = nullptr;

    InstanceData(InstanceData* LastInstanceData, VulkanContext* vulkanRef)
    {
        vulkanContext = vulkanRef;

        if (LastInstanceData)
        {
            glm::vec3 PositionOffset = glm::vec3(2, 0, 0);

            Position = LastInstanceData->Position + PositionOffset;
            Scale = LastInstanceData->Scale;
            Rotation = LastInstanceData->Rotation;

        }

        UpdateModelMatrix();
    }

    glm::vec3 GetPostion() const { return Position; }
    glm::vec3 GetRotation() const { return Rotation; }
    glm::vec3 GetScale() const { return Scale; }
    glm::mat4 GetModelMatrix() const { return modelMatrix; }

    void SetPostion(glm::vec3 NewPosition)
    {
        Position = NewPosition;
        UpdateModelMatrix();
    }

     void SetRotation(glm::vec3 NewRotation)
    {
        Rotation = NewRotation;
        UpdateModelMatrix();
    }

     void SetScale(glm::vec3 NewScale)
    {
        Scale = NewScale;
        UpdateModelMatrix();
    }

   virtual void SetModelMatrix(glm::mat4 NewModelMatrix)
    {
        modelMatrix = NewModelMatrix;
        BreakDownAndUpdateModelMatrix();

    }

   virtual void UpdateModelMatrix()
    {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, Position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale (modelMatrix, Scale);

        vulkanContext->ResetTemporalAccumilation();
    }

    void BreakDownAndUpdateModelMatrix()
    {

        glm::vec3 Newscale;
        glm::quat Newrotation;
        glm::vec3 Newtranslation;
        glm::vec3 Newskew;
        glm::vec4 Newperspective;
        glm::decompose(modelMatrix, Newscale, Newrotation, Newtranslation, Newskew, Newperspective);

        Position = Newtranslation;
        Rotation = glm::degrees(glm::eulerAngles(glm::conjugate(Newrotation)));
        Scale = Newscale;
        vulkanContext->ResetTemporalAccumilation();

    }

private:

    glm::vec3 Position = glm::vec3(1);
    glm::vec3 Scale = glm::vec3(1);
    glm::vec3 Rotation = glm::vec3(1);
    glm::mat4 modelMatrix;
};

//////////////////////////////////////////////////

struct VertexData {
    alignas(16) glm::mat4 ViewMatrix;
    alignas(16) glm::mat4 ProjectionMatrix;
};

struct Model_GPU_InstanceData {
    alignas(16) glm::mat4 modelMatrix;
    glm::vec4 bCubeMapReflection_bScreenSpaceReflectionWithPadding;
};

struct Model_InstanceData : public InstanceData {


    std::shared_ptr<Model_GPU_InstanceData> gpu_InstanceData = std::make_shared<Model_GPU_InstanceData>();

    Model_InstanceData(Model_InstanceData* LastInstanceData, VulkanContext* vulkanRef) :InstanceData(LastInstanceData, vulkanRef)
    {
        if (LastInstanceData)
        {
            CubeMapReflectiveSwitch (LastInstanceData->bCubeMapReflection);
            ScreenSpaceReflectiveSwitch(LastInstanceData->bScreenSpaceReflection);
            gpu_InstanceData->modelMatrix = GetModelMatrix();

        }

        UpdateGPU_ReflectionFlags();
    }

    void SetModelMatrix(glm::mat4 NewModelMatrix) override
    {
        InstanceData::SetModelMatrix(NewModelMatrix);
        gpu_InstanceData->modelMatrix = NewModelMatrix;

    }

     void UpdateModelMatrix() override
    {
         InstanceData::UpdateModelMatrix();
         gpu_InstanceData->modelMatrix = GetModelMatrix();
    }

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


    bool bCubeMapReflection = true;
    bool bScreenSpaceReflection = false;
};


struct Light_GPU_InstanceData {
    alignas(16) glm::mat4 modelMatrix;
    glm::vec4 Color;
};

struct Light_InstanceData : public InstanceData {

    std::shared_ptr<Light_GPU_InstanceData> gpu_InstanceData = std::make_shared<Light_GPU_InstanceData>();

    Light_InstanceData(Light_InstanceData* LastInstanceData, VulkanContext* vulkanRef) :InstanceData(LastInstanceData, vulkanRef)
    {
        if (LastInstanceData)
        {

            Color          = LastInstanceData->Color;
            LightType      = LastInstanceData->LightType;
            CastShadow     = LastInstanceData->CastShadow;
            LightIntensity = LastInstanceData->LightIntensity;

        }
    }

    void SetLightColor(glm::vec3 LightColor)
    {
        Color = LightColor;
    }


    void SetLightType(int lightType)
    {
        LightType = lightType;
    }


    void SetCastShadow(bool castShadow)
    {
        CastShadow = castShadow;
    }

    void SetLightIntensity(float lightIntensity)
    {
        LightIntensity = lightIntensity;
    }


    int        LightType      = 1;
    int        CastShadow     = 0;
    float      LightIntensity = 5;
    glm::vec3  Color = glm::vec3(1);
};

