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

} // namespace minecraft
