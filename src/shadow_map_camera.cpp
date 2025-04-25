#include "shadow_map_camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>

namespace minecraft {

void ShadowMapCamera::update(const glm::vec3 &lightDirection, const Camera &camera)
{
    std::array<float, ShadowMapCascadeCount + 1> ClipSpaceZSplits;
    ClipSpaceZSplits.front() = -1.0f;
    ClipSpaceZSplits.back() = 1.0f;
    {
        // Logarithmic split scheme:
        // https://computergraphics.stackexchange.com/questions/13026/cascaded-shadow-mapping-csm-partitioning-the-frustum-to-a-nearly-1-by-1-mappi
        const auto zRatio{camera.far() / camera.near()};
        for (const auto cascadeIndex : std::views::iota(1, ShadowMapCascadeCount)) {
            const auto splitRatio{static_cast<float>(cascadeIndex)
                                  / static_cast<float>(ShadowMapCascadeCount)};
            const auto zSplit{camera.near() * std::pow(zRatio, splitRatio)};
            const glm::vec4 viewSpaceSplit{0.0f, 0.0f, -zSplit, 1.0f};
            const auto clipSpaceSplit{camera.projectionMatrix() * viewSpaceSplit};
            ClipSpaceZSplits[cascadeIndex] = clipSpaceSplit.z / clipSpaceSplit.w;
        }
    }

    // Start with this view matrix and adjust it later for each cascade.
    // The light direction vector points towards the light source.
    const auto &cameraPosition{camera.pose().position()};
    const auto baseShadowViewMatrix{
        glm::lookAt(cameraPosition, cameraPosition - lightDirection, glm::vec3{0.0f, 1.0f, 0.0f})};

    const auto viewProjectionMatrixInverse{glm::inverse(camera.viewProjectionMatrix())};

    for (const auto cascadeIndex : std::views::iota(0, ShadowMapCascadeCount)) {
        constexpr auto Infinity{std::numeric_limits<float>::infinity()};
        glm::vec3 minPoint{Infinity};
        glm::vec3 maxPoint{-Infinity};

        for (const auto clipSpaceX : {-1.0f, 1.0f}) {
            for (const auto clipSpaceY : {-1.0f, 1.0f}) {
                for (const auto clipSpaceZ : {
                         ClipSpaceZSplits[cascadeIndex],
                         ClipSpaceZSplits[cascadeIndex + 1],
                     }) {
                    const auto worldSpacePoint{
                        viewProjectionMatrixInverse
                        * glm::vec4{clipSpaceX, clipSpaceY, clipSpaceZ, 1.0f}};
                    const auto shadowViewSpacePoint{
                        glm::vec3{baseShadowViewMatrix * (worldSpacePoint / worldSpacePoint.w)}};
                    minPoint = glm::min(minPoint, shadowViewSpacePoint);
                    maxPoint = glm::max(maxPoint, shadowViewSpacePoint);
                }
            }
        }

        // Add a small margin to the bounding box.
        minPoint -= 1.0f;
        maxPoint += 1.0f;

        // Rescale the Z dimension to fit shadow-casting objects outside the camera's view frustum.
        // This does not have a large impact on the shadow map's precision as another unscaled depth
        // buffer is used for shadow mapping.
        {
            const auto middleZ{(minPoint.z + maxPoint.z) * 0.5f};
            const auto halfZScale{std::max((maxPoint.z - minPoint.z) * 0.5f, 128.0f)};
            minPoint.z = middleZ - halfZScale;
            maxPoint.z = middleZ + halfZScale;
        }

        // Shift the Z values to ensure they are always negative.
        auto &shadowViewMatrix{_viewMatrices[cascadeIndex]};
        shadowViewMatrix = baseShadowViewMatrix;
        shadowViewMatrix[3][2] -= maxPoint.z;
        minPoint.z -= maxPoint.z;
        maxPoint.z = 0.0f;

        // Use orthographic projection so that the depth is linear. Note that near and far values
        // have inverted signs.
        _projectionMatrices[cascadeIndex]
            = glm::ortho(minPoint.x, maxPoint.x, minPoint.y, maxPoint.y, -maxPoint.z, -minPoint.z);
    }
};

glm::vec2 ShadowMapCamera::getDepthBlurScale(const int cascadeIndex) const
{
    const glm::vec2 viewSpaceScale{0.2f};
    const auto &projectionMatrix{_projectionMatrices[cascadeIndex]};
    const auto clipSpaceScale{viewSpaceScale
                              * glm::vec2{projectionMatrix[0][0], projectionMatrix[1][1]}};
    const auto ndcScale{clipSpaceScale * 0.5f};
    return ndcScale;
}

} // namespace minecraft
