#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

minecraft::Camera::Camera(const Pose &pose, const int viewportWidth, const int viewportHeight)
    : _pose{pose}
    , _fieldOfViewY{45.0f}
    , _nearPlane{0.1f}
    , _farPlane{1000.0f}
{
    setViewportSize(viewportWidth, viewportHeight);
}

auto minecraft::Camera::pose() const -> const Pose &
{
    return _pose;
}

auto minecraft::Camera::setPose(const Pose &pose) -> void
{
    _pose = pose;
}

auto minecraft::Camera::setViewportSize(const int width, const int height) -> void
{
    _aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    updateProjectionMatrix();
}

auto minecraft::Camera::viewProjectionMatrix() const -> glm::mat4
{
    return _projectionMatrix * _pose.viewMatrix();
}

auto minecraft::Camera::updateProjectionMatrix() -> void
{
    _projectionMatrix = glm::perspective(glm::radians(_fieldOfViewY),
                                         _aspectRatio,
                                         _nearPlane,
                                         _farPlane);
}
