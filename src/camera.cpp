#include "camera.h"

#include <glm/gtc/quaternion.hpp>

#include <initializer_list>

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
        glm::vec3 right{_pose.right()};
        glm::vec3 up{_pose.up()};
        glm::vec3 forward{_pose.forward()};
        for (const auto axis : {&right, &up, &forward}) {
            axis->y = -axis->y;
        }
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
