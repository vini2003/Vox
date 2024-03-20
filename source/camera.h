//
// Created by vini2003 on 20/03/2024.
//

#ifndef CAMERA_H
#define CAMERA_H
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "constants.h"


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
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const;

    void handleKeyboardInput(GLFWwindow *window, float deltaTime);
    void handleMouseInput(GLFWwindow* window, GLboolean constrainPitch = true);
    void handleMouseScroll(GLFWwindow * window, double x, double y);

    void updateCameraVectors();

    [[nodiscard]] glm::vec3 getPosition() const;

    void setPosition(const glm::vec3 &position);

    [[nodiscard]] glm::vec3 getFront() const;

    void setFront(const glm::vec3 &front);

    [[nodiscard]] glm::vec3 getUp() const;

    void setUp(const glm::vec3 &up);

    [[nodiscard]] glm::vec3 getRight() const;

    void setRight(const glm::vec3 &right);

    [[nodiscard]] glm::vec3 getWorldUp() const;

    void setWorldUp(const glm::vec3 &worldUp);

    [[nodiscard]] float getYaw() const;

    void setYaw(float yaw);

    [[nodiscard]] float getPitch() const;

    void setPitch(float pitch);

    [[nodiscard]] float getMovementSpeed() const;

    void setMovementSpeed(float movementSpeed);

    [[nodiscard]] float getMouseSensitivity() const;

    void setMouseSensitivity(float mouseSensitivity);

    [[nodiscard]] float getZoom() const;

    void setZoom(float zoom);
};



#endif //CAMERA_H
