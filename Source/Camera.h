#pragma once
#include <iostream>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;
class Camera {
public:
    
    Camera(uint32_t SwapchainHeight, uint32_t SwapchainWidth, GLFWwindow* window);

    // Initialize camera with specific parameters
    void Initialize(float fov = 90.0f, float nearClip = 0.1f, float farClip = 500.0f);

    // Update camera matrices (to be called every frame)
    void Update(float deltaTime);
    void UpdateJitter(float jitterX, float jitterY);
    void updateJitterMat(uint32_t frameIndex, int numSamples, int width, int height);

    // Getters for the view and projection matrices
    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetPrevViewMatrix() const;
    const glm::mat4& GetProjectionMatrix() const;
    const glm::mat4& GetPrevProjectionMatrix() const;
    const glm::mat4& GetJitteredProjectionMatrix() const { return jitteredProjectionMatrix; }
    const glm::mat4 GetjitterMat() const { return jitterMat_; }
    const glm::vec2 GetjitterInPixelSpace() const { return jitterVal_; }


    // Get camera properties
    const glm::vec3& GetPosition() const;
    const glm::vec3& GetForward() const;
    const glm::vec3& GetRight() const;
    const glm::vec3& GetUp() const;
    float GetFOV() const;
    float GetNearClip() const;
    float GetFarClip() const;

    // Camera control settings
    void SetMovementSpeed(float speed);
    void SetSwapchainHeight(float SwapchainHeight);
    void SetSwapchainWidth(float SwapchainWidth);
    void SetMouseSensitivity(float sensitivity);
    void SetFOV(float fov);
    void SetPosition(const glm::vec3& newPosition);
    void SetRotation(float newYaw, float newPitch);
    void OnFrameStart();

    glm::vec3 position;
    float pitch;
    float yaw;
    bool mouseCaptured;

private:
    // Camera properties
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Camera angles for rotation


    // Camera settings
    float movementSpeed;
    float mouseSensitivity;
    float fov;
    float nearClip;
    float farClip;

    // Camera matrices
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;


    glm::mat4 Prev_viewMatrix;
    glm::mat4 Prev_projectionMatrix;

    glm::mat4 jitteredProjectionMatrix;
    /// WindowRef
    GLFWwindow* window;

    // Internal methods
    void UpdateCameraVectors();
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    // Window/monitor info
    float swapchainHeight;
    float swapchainWidth;

    double lastMouseX;
    double lastMouseY;
    bool firstMouse;


    glm::mat4 jitterMat_{ 1.0f };
    glm::vec2 jitterVal_;

	int frameIndex = 0;
};