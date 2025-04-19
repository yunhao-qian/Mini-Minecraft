#include "camera.h"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <initializer_list>

namespace minecraft {

Camera Camera::createReflectionCamera(const float waterElevation) const
{
    const glm::vec3 position{
        _pose.position().x,
        2.0f * waterElevation - _pose.position().y,
        _pose.position().z,
    };
    glm::quat orientation;
    {
        auto right{_pose.right()};
        auto forward{_pose.forward()};
        for (const auto axis : {&right, &forward}) {
            axis->y = -axis->y;
        }
        const auto up{glm::cross(right, forward)};
        const auto rotation{glm::mat3{right, up, -forward}};
        orientation = glm::quat_cast(rotation);
    }

    Pose pose{position};
    pose.setOrientation(orientation);

    Camera camera{*this};
    camera.setPose(pose);
    // Increase the field of view because screen-space reflections and refractions can go outside of
    // the camera's view frustum.
    camera.setFieldOfViewY(std::min(_fieldOfViewY * 1.5f, std::max(_fieldOfViewY, 160.0f)));
    return camera;
}

Camera Camera::createRefractionCamera(const float waterElevation, const float refractiveIndex) const
{
    // Approximates the apparent shift in camera position due to refraction at the water surface.
    // Note that the refracted rays do not converge to a single point, so this is a simplified
    // model.
    auto position{_pose.position()};
    if (position.y >= waterElevation) {
        position.y = waterElevation + (position.y - waterElevation) * refractiveIndex;
    } else {
        position.y = waterElevation - (waterElevation - position.y) / refractiveIndex;
    }

    Pose pose{position};
    pose.setOrientation(_pose.orientation());

    Camera camera{*this};
    camera.setPose(pose);
    camera.setFieldOfViewY(std::min(_fieldOfViewY * 1.5f, std::max(_fieldOfViewY, 160.0f)));
    return camera;
}

bool Camera::isInViewFrustum(const AlignedBox3D &box) const
{
    for (const auto &plane : _frustumPlanes) {
        const glm::vec3 normal{plane};
        const glm::vec3 innerMostPoint{
            normal.x >= 0.0f ? box.maxPoint().x : box.minPoint().x,
            normal.y >= 0.0f ? box.maxPoint().y : box.minPoint().y,
            normal.z >= 0.0f ? box.maxPoint().z : box.minPoint().z,
        };
        if (glm::dot(normal, innerMostPoint) + plane.w < 0.0f) {
            return false;
        }
    }
    return true;
}

void Camera::updateViewProjectionMatrix()
{
    _viewProjectionMatrix = _projectionMatrix * _pose.viewMatrix();

    // Clip-space plane: [a' b' c' d'] [x' y' z' w']^T = 0
    // View-projection matrix: [x' y' z' w']^T = VP [x y z 1]^T
    // Put the two together: ([a' b' c' d'] VP) [x y z 1]^T = 0
    // Therefore, the parameters of a view frustum plane are given by [a' b' c' d'] VP.
    const auto m{glm::transpose(_viewProjectionMatrix)};
    _frustumPlanes[0] = m[3] + m[0]; // Left
    _frustumPlanes[1] = m[3] - m[0]; // Right
    _frustumPlanes[2] = m[3] + m[1]; // Bottom
    _frustumPlanes[3] = m[3] - m[1]; // Top
    _frustumPlanes[4] = m[3] + m[2]; // Near
    _frustumPlanes[5] = m[3] - m[2]; // Far
}

} // namespace minecraft
