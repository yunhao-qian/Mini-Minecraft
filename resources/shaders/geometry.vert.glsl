#version 410 core

#include "block_face.glsl"
#include "block_type.glsl"
#include "uniform_buffer_data.glsl"
#include "water_wave.glsl"

uniform int u_cameraIndex;

layout(location = 0) in ivec3 a_faceOrigin;
layout(location = 1) in int a_faceIndex;
layout(location = 2) in int a_textureIndex;
layout(location = 3) in int a_blockType;
layout(location = 4) in int a_mediumType;

flat out mat3 v_viewSpaceTBNMatrix;
flat out int v_textureIndex;
flat out int v_blockType;
flat out int v_mediumType;
out vec3 v_worldSpacePosition;
out vec2 v_textureCoords;
out float v_waterLevel;

vec2 randomOffset(float frequency)
{
    float discreteTime = floor(u_time * frequency) / frequency;
    vec2 offset = fract(sin(discreteTime * vec2(127.1, 269.5)) * 43758.5453);
    vec2 discreteOffset = floor(offset * 16.0) / 16.0;
    return discreteOffset;
}

void main()
{
    v_viewSpaceTBNMatrix = mat3(u_viewMatrices[u_cameraIndex]) * FaceTBNMatrices[a_faceIndex];
    v_textureIndex = a_textureIndex;
    v_blockType = a_blockType;
    v_mediumType = a_mediumType;

    ivec2 textureCoords = FaceTextureCoords[gl_VertexID];

    v_worldSpacePosition = vec3(a_faceOrigin + textureCoords.x * FaceTangents[a_faceIndex]
                                + textureCoords.y * FaceBitangents[a_faceIndex]);
    if (a_blockType == BlockTypeWater) {
        v_worldSpacePosition.y += getWaterWaveOffset(v_worldSpacePosition.xz, u_time);
    }

    v_textureCoords = vec2(textureCoords);
    if (a_blockType == BlockTypeLava) {
        v_textureCoords += randomOffset(4.0);
    }

    // TODO: Replace the hardcoded water level.
    if (a_blockType == BlockTypeWater || a_faceOrigin.y < 137 || a_faceOrigin.y > 138) {
        // Water block faces do not need the water level. For block faces far away from the water.
        // the precise water level is not important. They only need to know if they are above or
        // below the water level.
        v_waterLevel = 137.5;
    } else {
        // Block faces close to the water need the precise water level to determine the actual
        // medium type.
        v_waterLevel = 138.0 + getWaterWaveOffset(v_worldSpacePosition.xz, u_time);
    }

    gl_Position = u_viewProjectionMatrices[u_cameraIndex] * vec4(v_worldSpacePosition, 1.0);
}
