#include "camera.h"
#include "glm/fwd.hpp"

#include <initializer_list>
#include <limits>

namespace minecraft {

std::pair<glm::mat4, glm::mat4> Camera::getDirectionalLightShadowViewProjectionMatrices(
    const glm::vec3 &direction) const
{
    // Find a view projection matrix for the directional light that covers the camera's view
    // frustum.

    // The light direction vector points towards the light source.
    // glm::vec3 position{_pose.position().x, 0.0, _pose.position().z};
    auto shadowViewMatrix{
        glm::lookAt(_pose.position(), _pose.position() - direction, glm::vec3{0.0f, 1.0f, 0.0f})};

    constexpr auto Infinity{std::numeric_limits<float>::infinity()};
    glm::vec3 minPoint{Infinity, Infinity, Infinity};
    glm::vec3 maxPoint{-Infinity, -Infinity, -Infinity};

    // We approximate the view frustum by checking the eight corners of the clip space cube.
    const auto viewProjectionMatrixInverse{glm::inverse(_projectionMatrix * _pose.viewMatrix())};
    for (const auto clipSpaceX : {-1.0f, 1.0f}) {
        for (const auto clipSpaceY : {-1.0f, 1.0f}) {
            for (const auto clipSpaceZ : {-1.0f, 1.0f}) {
                const auto worldSpacePoint{viewProjectionMatrixInverse
                                           * glm::vec4{clipSpaceX, clipSpaceY, clipSpaceZ, 1.0f}};
                const auto shadowViewSpacePoint{
                    glm::vec3{shadowViewMatrix * (worldSpacePoint / worldSpacePoint.w)}};
                minPoint = glm::min(minPoint, shadowViewSpacePoint);
                maxPoint = glm::max(maxPoint, shadowViewSpacePoint);
            }
        }
    }

    // Add a small margin to the bounding box.
    minPoint -= 1.0f;
    maxPoint += 1.0f;

    // Shift the Z values to ensure they are always negative.
    shadowViewMatrix[3][2] -= maxPoint.z;
    minPoint.z -= maxPoint.z;
    maxPoint.z = 0.0f;

    // Use orthographic projection so that the depth is linear.
    // Near and far planes are specified in the negative Z-direction.
    const auto shadowProjectionMatrix{
        glm::ortho(minPoint.x, maxPoint.x, minPoint.y, maxPoint.y, -maxPoint.z, -minPoint.z)};

    return {shadowViewMatrix, shadowProjectionMatrix};
}

} // namespace minecraft
