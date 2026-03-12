#include "Camera.h"
#include <GLFW/glfw3.h>

Camera::Camera(uint32_t SwapchainHeight, uint32_t SwapchainWidth, GLFWwindow* Window) :
    position(-8.56827, 2.08025, 0.447241),
    worldUp(0.0f, 1.0f, 0.0f),
    pitch(-4.5),
    yaw(-11),
    movementSpeed(15.0f),
    mouseSensitivity(0.1f),
    fov(100.0f),
    nearClip(0.1f),
    farClip(200.0f),
    firstMouse(true),
    mouseCaptured(false),
    window(Window),
    swapchainHeight(SwapchainHeight),
    swapchainWidth(SwapchainWidth)
{
    Initialize();
}

void Camera::Initialize(float Fov, float NearClip, float FarClip) {
    fov = Fov;
    nearClip = NearClip;
    farClip = FarClip;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    UpdateCameraVectors();
    UpdateViewMatrix();       

   
    UpdateProjectionMatrix(); 

    Prev_viewMatrix = viewMatrix;
    Prev_projectionMatrix = projectionMatrix;
}

void Camera::Update(float deltaTime) {

    Prev_viewMatrix = viewMatrix;
    Prev_projectionMatrix = projectionMatrix;

    frameIndex = (frameIndex + 1) % 64;
    halton(frameIndex);

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

    UpdateProjectionMatrix();
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
    float aspectRatio = static_cast<float>(swapchainWidth) / static_cast<float>(swapchainHeight);
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);

}

const glm::mat4& Camera::GetViewMatrix() const {
    return viewMatrix;
}

const glm::mat4& Camera::GetPrevViewMatrix() const {
    return Prev_viewMatrix;
}

const glm::mat4& Camera::GetProjectionMatrix() const {
    return projectionMatrix;
}

const glm::mat4& Camera::GetPrevProjectionMatrix() const {
    return Prev_projectionMatrix;
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

float Camera::GetFOV() const {
    return fov;
}

float Camera::GetNearClip() const {
    return nearClip;
}

float Camera::GetFarClip() const {
    return farClip;
}
void Camera::SetMovementSpeed(float speed) {
    movementSpeed = speed;
}

void Camera::SetSwapchainHeight(float SwapchainHeight) {

    swapchainHeight = SwapchainHeight;
    UpdateProjectionMatrix();
}

void Camera::SetSwapchainWidth(float SwapchainWidth) {
    swapchainWidth = SwapchainWidth;
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

void Camera::SetPosition(const glm::vec3& newPosition) {
    position = newPosition;
    UpdateViewMatrix();
}

void Camera::SetRotation(float newYaw, float newPitch) {
    yaw = newYaw;
    pitch = newPitch;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    UpdateCameraVectors();
    UpdateViewMatrix();
}

void Camera::OnFrameStart() {
    Prev_viewMatrix = viewMatrix;
    Prev_projectionMatrix = projectionMatrix;
}


// #DLSS_RR
// halton low discrepancy sequence, from https://www.shadertoy.com/view/wdXSW8
void Camera::halton(int index)
{
    const glm::vec2 coprimes = glm::vec2(2.0F, 3.0F);
    glm::vec2       s = glm::vec2(index, index);
    glm::vec4       a = glm::vec4(1, 1, 0, 0);
    while (s.x > 0. && s.y > 0.)
    {
        a.x = a.x / coprimes.x;
        a.y = a.y / coprimes.y;
        a.z += a.x * fmod(s.x, coprimes.x);
        a.w += a.y * fmod(s.y, coprimes.y);
        s.x = floorf(s.x / coprimes.x);
        s.y = floorf(s.y / coprimes.y);
    }
    Jitter = glm::vec2(a.z, a.w);
}
