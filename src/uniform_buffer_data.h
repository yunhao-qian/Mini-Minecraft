#ifndef MINECRAFT_UNIFORM_BUFFER_DATA_H
#define MINECRAFT_UNIFORM_BUFFER_DATA_H

#include "constants.h"

#include <glm/glm.hpp>

namespace minecraft {

// This struct should be in sync with uniform_buffer_data.glsl.
struct UniformBufferData
{
    float time;

    float cameraNear;
    float cameraFar;

    // Align floats to 16 bytes
    float padding{0.0f};

    glm::mat4 viewMatrices[3];
    glm::mat4 viewProjectionMatrices[3];

    glm::mat4 shadowViewMatrices[ShadowMapCascadeCount];
    glm::mat4 shadowViewProjectionMatrices[ShadowMapCascadeCount];

    glm::mat4 mainToShadowViewMatrices[ShadowMapCascadeCount];
    glm::mat4 mainToShadowViewProjectionMatrices[ShadowMapCascadeCount];

    glm::mat4 viewMatrixInverse;
    glm::mat4 projectionMatrixInverse;

    glm::mat4 reflectionProjectionMatrix;
    glm::mat4 mainToReflectionViewMatrix;
    glm::mat4 reflectionToMainViewMatrix;

    glm::mat4 refractionProjectionMatrix;
    glm::mat4 mainToRefractionViewMatrix;
    glm::mat4 refractionToMainViewMatrix;
};

} // namespace minecraft

#endif // MINECRAFT_UNIFORM_BUFFER_DATA_H
