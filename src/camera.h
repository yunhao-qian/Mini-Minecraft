#ifndef MINI_MINECRAFT_CAMERA_H
#define MINI_MINECRAFT_CAMERA_H

#include "pose.h"

#include <glm/glm.hpp>

namespace minecraft {

class Camera
{
public:
    Camera(const Pose &pose, const int viewportWidth, const int viewportHeight);

    auto pose() const -> const Pose &;
    auto setPose(const Pose &pose) -> void;

    auto setViewportSize(const int width, const int height) -> void;

    auto viewProjectionMatrix() const -> glm::mat4;

private:
    auto updateProjectionMatrix() -> void;

    Pose _pose;
    float _fieldOfViewY;
    float _aspectRatio;
    float _nearPlane;
    float _farPlane;
    glm::mat4 _projectionMatrix;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_CAMERA_H
