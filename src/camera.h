#ifndef MINI_MINECRAFT_CAMERA_H
#define MINI_MINECRAFT_CAMERA_H

#include "pose.h"

#include <glm/glm.hpp>

namespace minecraft {

class Camera
{
public:
    Camera(const Pose &pose,
           const int width,
           const int height,
           const float fieldOfViewY = 45.0f,
           const float near = 0.1f,
           const float far = 1000.0f);

    const Pose &pose() const;
    void setPose(const Pose &pose);

    float fieldOfViewY() const;
    void setFieldOfViewY(const float fieldOfViewY);

    float near() const;
    float far() const;

    void resizeViewport(const int width, const int height);

    const glm::mat4 &projectionMatrix() const;

private:
    void updateProjectionMatrix();

    Pose _pose;
    float _fieldOfViewY;
    float _aspect;
    float _near;
    float _far;
    glm::mat4 _projectionMatrix;
};

inline Camera::Camera(const Pose &pose,
                      const int width,
                      const int height,
                      const float fieldOfViewY,
                      const float near,
                      const float far)
    : _pose{pose}
    , _fieldOfViewY{fieldOfViewY}
    , _aspect{}
    , _near{near}
    , _far{far}
{
    resizeViewport(width, height);
}

inline const Pose &Camera::pose() const
{
    return _pose;
}

inline void Camera::setPose(const Pose &pose)
{
    _pose = pose;
}

inline float Camera::fieldOfViewY() const
{
    return _fieldOfViewY;
}

inline void Camera::setFieldOfViewY(const float fieldOfViewY)
{
    _fieldOfViewY = fieldOfViewY;
    updateProjectionMatrix();
}

inline float Camera::near() const
{
    return _near;
}

inline float Camera::far() const
{
    return _far;
}

inline void Camera::resizeViewport(const int width, const int height)
{
    _aspect = static_cast<float>(width) / static_cast<float>(height);
    updateProjectionMatrix();
}

inline const glm::mat4 &Camera::projectionMatrix() const
{
    return _projectionMatrix;
}

inline void Camera::updateProjectionMatrix()
{
    _projectionMatrix = glm::perspective(glm::radians(_fieldOfViewY), _aspect, _near, _far);
}

} // namespace minecraft

#endif // MINI_MINECRAFT_CAMERA_H
