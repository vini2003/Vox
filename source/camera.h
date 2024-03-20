//
// Created by vini2003 on 20/03/2024.
//

#ifndef CAMERA_H
#define CAMERA_H
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "constants.h"

namespace vox {
    class Camera {
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 worldUp;

        float yaw;
        float pitch;

        float movementSpeed;
        float mouseSensitivity;
        float zoom;

        double lastX = DEFAULT_WIDTH / 2.0;
        double lastY = DEFAULT_HEIGHT / 2.0;

        bool firstMouse = true;
        bool reentryMouse = false;

    public:
        Camera(
            const glm::vec3 &position = glm::vec3(0.0f, 0.0f, 0.0f),
            const glm::vec3 &up = glm::vec3(0.0f, 0.0f, 1.0f),
            float yaw = -90.0f,
            float pitch = 0.0f)
            : position(position),
              front(glm::vec3(0.0f, 0.0f, -1.0f)),
              worldUp(up),
              yaw(yaw),
              pitch(pitch),
              movementSpeed(2.5f),
              mouseSensitivity(0.1f),
              zoom(45.0f) {
            updateVecs();
        }

        [[nodiscard]] glm::mat4 getViewMatrix() const;
        [[nodiscard]] glm::mat4 getProjectionMatrix(float aspectRatio) const;

        void handleKeyboardInput(GLFWwindow *window, float deltaTime);
        void handleMouseInput(GLFWwindow* window, GLboolean constrainPitch = true);
        void handleMouseScroll(GLFWwindow * window, double x, double y);

        void updateVecs();

        [[nodiscard]] glm::vec3 getPosition() const;
        [[nodiscard]] glm::vec3 getFront() const;
        [[nodiscard]] glm::vec3 getUp() const;
        [[nodiscard]] glm::vec3 getRight() const;
        [[nodiscard]] glm::vec3 getWorldUp() const;
        [[nodiscard]] float getYaw() const;
        [[nodiscard]] float getPitch() const;
        [[nodiscard]] float getMovementSpeed() const;
        [[nodiscard]] float getMouseSensitivity() const;
        [[nodiscard]] float getZoom() const;

        void setPosition(const glm::vec3 &position);
        void setFront(const glm::vec3 &front);
        void setUp(const glm::vec3 &up);
        void setRight(const glm::vec3 &right);
        void setWorldUp(const glm::vec3 &worldUp);
        void setYaw(float yaw);
        void setPitch(float pitch);
        void setMovementSpeed(float movementSpeed);
        void setMouseSensitivity(float mouseSensitivity);
        void setZoom(float zoom);
    };
}

#endif //CAMERA_H
