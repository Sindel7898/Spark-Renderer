#include "Camera.h"
#include <GLFW/glfw3.h>

Camera::Camera(uint32_t SwapChainHeight, uint32_t SwapChainWidth, GLFWwindow* Window) :
    position(0.0f, 0.0f, 4.0f),
    worldUp(0.0f, 1.0f, 0.0f),
    pitch(0.0f),
    yaw(-90.0f),
    movementSpeed(15.0f),
    mouseSensitivity(0.1f),
    fov(100.0f),
    nearClip(0.0001f),
    farClip(200.0f),
    firstMouse(true),
    mouseCaptured(false),
    window(Window),
    swapChainHeight(SwapChainHeight),
    swapChainWidth(SwapChainWidth)
{
    Initialize();
}

void Camera::Initialize(float Fov, float NearClip, float FarClip) {
    fov = Fov;
    nearClip = NearClip;
    farClip = FarClip;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    UpdateCameraVectors();
    UpdateProjectionMatrix();
}

void Camera::Update(float deltaTime) {

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && !mouseCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
        firstMouse = true; 
    }
    else if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && mouseCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured = false;
    }

    if (mouseCaptured)
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (firstMouse) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }

        float xOffset = static_cast<float>(mouseX - lastMouseX);
        float yOffset = static_cast<float>(lastMouseY - mouseY);

        // Reset mouse position to center for infinite movement
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glfwSetCursorPos(window, width / 2, height / 2);
        lastMouseX = width / 2;
        lastMouseY = height / 2;


        xOffset *= mouseSensitivity;
        yOffset *= mouseSensitivity;

        yaw += xOffset;
        pitch += yOffset;

        // Constrain pitch to avoid gimbal lock
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        // Update camera vectors
        UpdateCameraVectors();

        // Handle keyboard input
        float velocity = movementSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            position += forward * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            position -= forward * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position -= right * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            position += right * velocity;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            position += worldUp * velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            position -= worldUp * velocity;

        UpdateViewMatrix();
    }
}

void Camera::UpdateCameraVectors() {
    // Calculate new forward vector
    glm::vec3 newForward;
    newForward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newForward.y = sin(glm::radians(pitch));
    newForward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    forward = glm::normalize(newForward);

    // Re-calculate right and up vectors
    right = glm::normalize(glm::cross(forward, worldUp));
    up = glm::normalize(glm::cross(right, forward));
}

void Camera::UpdateViewMatrix() {

    viewMatrix = glm::lookAt(position, position + forward, up);
}

void Camera::UpdateProjectionMatrix() {
    float aspectRatio = swapChainWidth / swapChainHeight;
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
}

const glm::mat4& Camera::GetViewMatrix() const {
    return viewMatrix;
}

const glm::mat4& Camera::GetProjectionMatrix() const {
    return projectionMatrix;
}

const glm::vec3& Camera::GetPosition() const {
    return position;
}

const glm::vec3& Camera::GetForward() const {
    return forward;
}

const glm::vec3& Camera::GetRight() const {
    return right;
}

const glm::vec3& Camera::GetUp() const {
    return up;
}

void Camera::SetMovementSpeed(float speed) {
    movementSpeed = speed;
}

void Camera::SetSwapChainHeight(float SwapChainHeight) {

    swapChainHeight = SwapChainHeight;
    UpdateProjectionMatrix();
}

void Camera::SetSwapChainWidth(float SwapChainWidth) {
    swapChainWidth = SwapChainWidth;
    UpdateProjectionMatrix();
}

void Camera::SetMouseSensitivity(float sensitivity)
{
    mouseSensitivity = sensitivity;

}

void Camera::SetFOV(float fov) {
    this->fov = fov;
    UpdateProjectionMatrix();
}