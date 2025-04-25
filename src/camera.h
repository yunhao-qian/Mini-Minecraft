#ifndef MINECRAFT_CAMERA_H
#define MINECRAFT_CAMERA_H

#include "aligned_box_3d.h"
#include "pose.h"

#include <glm/glm.hpp>

#include <array>

namespace minecraft {

class Camera
{
public:
    Camera(const Pose &pose,
           const int width,
           const int height,
           const float fieldOfViewY = 45.0f,
           const float near = 0.1f,
           const float far = 1024.0f)
        : _pose{pose}
        , _fieldOfViewY{fieldOfViewY}
        , _aspect{}
        , _near{near}
        , _far{far}
    {
        resizeViewport(width, height);
    }

    const Pose &pose() const { return _pose; }

    void setPose(const Pose &pose)
    {
        _pose = pose;
        updateViewProjectionMatrix();
    }

    float fieldOfViewY() const { return _fieldOfViewY; }

    void setFieldOfViewY(const float fieldOfViewY)
    {
        _fieldOfViewY = fieldOfViewY;
        updateProjectionMatrix();
    }

    float near() const { return _near; }

    float far() const { return _far; }

    void resizeViewport(const int width, const int height)
    {
        _aspect = static_cast<float>(width) / static_cast<float>(height);
        updateProjectionMatrix();
    }

    const glm::mat4 viewMatrix() const { return _pose.viewMatrix(); }

    const glm::mat4 &projectionMatrix() const { return _projectionMatrix; }

    const glm::mat4 &viewProjectionMatrix() const { return _viewProjectionMatrix; }

    Camera createReflectionCamera(const float waterLevel) const;

    Camera createRefractionCamera(const float waterLevel, const float refractiveIndex) const;

    bool isInViewFrustum(const AlignedBox3D &box) const;

private:
    void updateProjectionMatrix()
    {
        _projectionMatrix = glm::perspective(glm::radians(_fieldOfViewY), _aspect, _near, _far);
        updateViewProjectionMatrix();
    }

    void updateViewProjectionMatrix();

    Pose _pose;
    float _fieldOfViewY;
    float _aspect;
    float _near;
    float _far;
    glm::mat4 _projectionMatrix;
    glm::mat4 _viewProjectionMatrix;
    std::array<glm::vec4, 6> _frustumPlanes;
};

} // namespace minecraft

#endif // MINECRAFT_CAMERA_H
