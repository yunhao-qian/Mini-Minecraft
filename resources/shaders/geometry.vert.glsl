#version 410 core

#include "block_face.glsl"
#include "block_type.glsl"
#include "water_wave.glsl"

uniform float u_time;
uniform mat4 u_viewMatrix;
uniform mat4 u_viewProjectionMatrix;

layout(location = 0) in ivec3 a_blockPosition;
layout(location = 1) in int a_faceIndex;
layout(location = 2) in int a_textureIndex;
layout(location = 3) in int a_blockType;
layout(location = 4) in int a_mediumType;

out vec3 v_worldSpacePosition;
flat out int v_textureIndex;
out vec2 v_textureCoords;
flat out vec3 v_viewSpaceTangent;
flat out vec3 v_viewSpaceBitangent;
flat out vec3 v_viewSpaceNormal;
flat out int v_blockType;
flat out int v_mediumType;
out float v_waterElevation;

vec2 randomOffset(float frequency)
{
    float discreteTime = floor(u_time * frequency) / frequency;
    vec2 offset = fract(sin(vec2(discreteTime * 127.1, discreteTime * 269.5)) * 43758.5453);
    vec2 discreteOffset = floor(offset * 16.0) / 16.0;
    return discreteOffset;
}

void main()
{
    ivec3 faceOrigin = a_blockPosition + FaceOrigins[a_faceIndex];
    ivec3 faceTangent = FaceTangents[a_faceIndex];
    ivec3 faceBitangent = FaceBitangents[a_faceIndex];
    ivec2 textureCoords = FaceTextureCoords[gl_VertexID];

    v_worldSpacePosition = vec3(faceOrigin + textureCoords.x * faceTangent
                                + textureCoords.y * faceBitangent);
    float waterWaveOffset = getWaterWaveOffset(v_worldSpacePosition.xz, u_time);
    if (a_blockType == BlockTypeWater) {
        v_worldSpacePosition.y += waterWaveOffset;
    }

    gl_Position = u_viewProjectionMatrix * vec4(v_worldSpacePosition, 1.0);

    v_textureIndex = a_textureIndex;
    v_textureCoords = vec2(textureCoords);
    if (a_blockType == BlockTypeLava) {
        v_textureCoords += randomOffset(4.0);
    }

    v_viewSpaceTangent = (u_viewMatrix * vec4(faceTangent, 0.0)).xyz;
    v_viewSpaceBitangent = (u_viewMatrix * vec4(faceBitangent, 0.0)).xyz;
    v_viewSpaceNormal = (u_viewMatrix * vec4(FaceNormals[a_faceIndex], 0.0)).xyz;

    v_blockType = a_blockType;
    v_mediumType = a_mediumType;

    // TODO: Replace the hardcoded water level.
    v_waterElevation = 138.0 + waterWaveOffset;
}
