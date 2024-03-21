//
// Created by vini2003 on 20/03/2024.
//

#include "camera.h"

#include <imgui.h>
#include <bits/stl_algo.h>
#include <glm/ext/matrix_clip_space.hpp>

namespace vox {
    glm::mat4 Camera::getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 Camera::getProjectionMatrix(const float aspectRatio) const {
        return glm::perspective(glm::radians(fieldOfView), aspectRatio, 0.1f, 100.0f);
    }

    void Camera::handleKeyboardInput(GLFWwindow* window, const float deltaTime) {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            const auto velocity = movementSpeed * deltaTime * 0.00005F;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                position += front * velocity;
            }

            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                position -= front * velocity;
            }

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                position -= right * velocity;
            }

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                position += right * velocity;
            }

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                position += worldUp * velocity;
            }

            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                position -= worldUp * velocity;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            const auto inputMode = glfwGetInputMode(window, GLFW_CURSOR);

            if (inputMode == GLFW_CURSOR_DISABLED) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else if (inputMode != GLFW_CURSOR_NORMAL) {
                // Must check again to avoid triggering reentry whilst the user is still holding escape,
                // or if they press it again but we've already handle it.
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

                reentryMouse = true;
            }
        }
    }

    void Camera::handleMouseInput(GLFWwindow *window, GLboolean constrainPitch) {
        double x, y;

        glfwGetCursorPos(window, &x, &y);

        // if mouse is inside window, and clicked, set glfw_cursor_disabled
        // otherwise, set glfw_cursor_normal
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
            if (glfwGetWindowAttrib(window, GLFW_HOVERED)) {
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    // check if the user is clicking in an imgui widget;
                    // if they are, don't change mode and immediatelly return
                    if (ImGui::GetIO().WantCaptureMouse) {
                        return;
                    }

                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

                    reentryMouse = true;
                }
            }

            return;
        }

        if (reentryMouse) {
            x = lastX;
            y = lastY;

            glfwSetCursorPos(window, x, y);

            reentryMouse = false;
        }

        if (firstMouse) {
            lastX = x;
            lastY = y;

            firstMouse = false;
        }


        auto deltaX = x - lastX;
        auto deltaY = lastY - y;

        lastX = x;
        lastY = y;

        deltaX *= mouseSensitivity;
        deltaY *= mouseSensitivity;

        // Yaw is subtracted because the camera UP vector is Z-positive.
        // If this were Y-positive, we'd sum instead.
        yaw -= deltaX;
        pitch += deltaY;

        if (constrainPitch) {
            if (pitch > 89.0f) {
                pitch = 89.0f;
            } else if (pitch < -89.0f) {
                pitch = -89.0f;
            }
        }

        updateVecs();
    }

    void Camera::handleMouseScroll(GLFWwindow *window, double x, double y) {
        if (y < 0) {
            movementSpeed -= 0.4f;
            movementSpeed = std::clamp(movementSpeed, 1.0f, 100.0f);
        } else if (y > 0) {
            movementSpeed += 0.4f;
            movementSpeed = std::clamp(movementSpeed, 1.0f, 100.0f);
        }
    }

    // Again, this doesn't look normal because the camera UP vector is Z-positive.
    // So we needed to adapt it.
    void Camera::updateVecs() {
        glm::vec3 front;

        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.z = sin(glm::radians(pitch));

        this->front = glm::normalize(front);

        this->right = glm::normalize(glm::cross(this->front, this->worldUp));
        this->up = glm::normalize(glm::cross(this->right, this->front));
    }

    glm::vec3 Camera::getPosition() const { return position; }
    glm::vec3 Camera::getFront() const { return front; }
    glm::vec3 Camera::getUp() const { return up; }
    glm::vec3 Camera::getRight() const { return right; }
    glm::vec3 Camera::getWorldUp() const { return worldUp; }
    float Camera::getYaw() const { return yaw; }
    float Camera::getPitch() const { return pitch; }
    float Camera::getMovementSpeed() const { return movementSpeed; }
    float Camera::getMouseSensitivity() const { return mouseSensitivity; }
    float Camera::getFieldOfView() const { return fieldOfView; }

    void Camera::setPosition(const glm::vec3 &position) { this->position = position; }
    void Camera::setFront(const glm::vec3 &front) { this->front = front; }
    void Camera::setUp(const glm::vec3 &up) { this->up = up; }
    void Camera::setRight(const glm::vec3 &right) { this->right = right; }
    void Camera::setWorldUp(const glm::vec3 &worldUp) { this->worldUp = worldUp; }
    void Camera::setYaw(const float yaw) { this->yaw = yaw; }
    void Camera::setPitch(const float pitch) { this->pitch = pitch; }
    void Camera::setMovementSpeed(const float movementSpeed) { this->movementSpeed = movementSpeed; }
    void Camera::setMouseSensitivity(const float mouseSensitivity) { this->mouseSensitivity = mouseSensitivity; }
    void Camera::setFieldOfView(const float fieldOfView) { this->fieldOfView = fieldOfView; }
}
