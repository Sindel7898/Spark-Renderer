#pragma once
#include <iostream>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;
class Camera {
public:
    
    Camera(uint32_t SwapChainHeight, uint32_t SwapChainWidth, GLFWwindow* window);

    // Initialize camera with specific parameters
    void Initialize(float fov = 90.0f, float nearClip = 0.1f, float farClip = 500.0f);

    // Update camera matrices (to be called every frame)
    void Update(float deltaTime);

    // Getters for the view and projection matrices
    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetPrevViewMatrix() const;
    const glm::mat4& GetProjectionMatrix() const;
    const glm::mat4& GetPrevProjectionMatrix() const;

    // Get camera properties
    const glm::vec3& GetPosition() const;
    const glm::vec3& GetForward() const;
    const glm::vec3& GetRight() const;
    const glm::vec3& GetUp() const;

    // Camera control settings
    void SetMovementSpeed(float speed);
    void SetSwapChainHeight(float SwapChainHeight);
    void SetSwapChainWidth(float SwapChainWidth);
    void SetMouseSensitivity(float sensitivity);
    void SetFOV(float fov);

private:
    // Camera properties
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Camera angles for rotation
    float pitch;
    float yaw;

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
    /// WindowRef
    GLFWwindow* window;

    // Internal methods
    void UpdateCameraVectors();
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    // Window/monitor info
    float swapChainHeight;
    float swapChainWidth;

    double lastMouseX;
    double lastMouseY;
    bool firstMouse;
    bool mouseCaptured;
};