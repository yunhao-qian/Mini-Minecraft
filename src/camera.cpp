#include "camera.h"

#include <glm/gtc/quaternion.hpp>

namespace minecraft {

Camera Camera::getReflectedCamera(const float waterElevation) const
{
    const glm::vec3 reflectedPosition{
        _pose.position().x,
        2.0f * waterElevation - _pose.position().y,
        _pose.position().z,
    };
    glm::quat reflectedOrientation;
    {
        const auto right{_pose.right()};
        const glm::vec3 forward{_pose.forward().x, -_pose.forward().y, _pose.forward().z};
        const auto up{glm::cross(right, forward)};
        const glm::mat3 reflectedRotation{right, up, -forward};
        reflectedOrientation = glm::quat_cast(reflectedRotation);
    }

    Pose reflectedPose{reflectedPosition};
    reflectedPose.setOrientation(reflectedOrientation);

    Camera reflectedCamera{*this};
    reflectedCamera.setPose(reflectedPose);
    return reflectedCamera;
}

} // namespace minecraft
