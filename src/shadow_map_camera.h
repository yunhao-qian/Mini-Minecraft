#ifndef MINECRAFT_SHADOW_MAP_CAMERA_H
#define MINECRAFT_SHADOW_MAP_CAMERA_H

#include "camera.h"

#include <glm/glm.hpp>

#include <array>

namespace minecraft {

class ShadowMapCamera
{
public:
    void update(const glm::vec3 &lightDirection, const Camera &camera);

    const glm::mat4 &viewMatrix(const int cascadeIndex) const
    {
        return _viewMatrices[cascadeIndex];
    }

    const glm::mat4 &projectionMatrix(const int cascadeIndex) const
    {
        return _projectionMatrices[cascadeIndex];
    }

    glm::vec2 getDepthBlurScale(const int cascadeIndex) const;

    static constexpr int CascadeCount{4};

private:
    std::array<glm::mat4, CascadeCount> _viewMatrices;
    std::array<glm::mat4, CascadeCount> _projectionMatrices;
};

} // namespace minecraft

#endif // MINECRAFT_SHADOW_MAP_CAMERA_H
