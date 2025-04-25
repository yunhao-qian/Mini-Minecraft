#version 410 core

#include "block_face.glsl"
#include "uniform_buffer_data.glsl"

uniform int u_cascadeIndex;

layout(location = 0) in ivec3 a_blockPosition;
layout(location = 1) in int a_faceIndex;

out float v_shadowViewSpaceDepth;

void main()
{
    ivec3 faceOrigin = a_blockPosition + FaceOrigins[a_faceIndex];
    ivec3 faceTangent = FaceTangents[a_faceIndex];
    ivec3 faceBitangent = FaceBitangents[a_faceIndex];
    ivec2 textureCoords = FaceTextureCoords[gl_VertexID];

    vec4 worldSpacePosition = vec4(vec3(faceOrigin + textureCoords.x * faceTangent
                                        + textureCoords.y * faceBitangent),
                                   1.0);

    mat4 shadowViewMatrix = u_shadowViewMatrices[u_cascadeIndex];
    v_shadowViewSpaceDepth = -(shadowViewMatrix * worldSpacePosition).z;

    mat4 shadowViewProjectionMatrix = u_shadowViewProjectionMatrices[u_cascadeIndex];
    gl_Position = shadowViewProjectionMatrix * worldSpacePosition;

    // Clamping the depth ensures that geometry outside the shadow frustum can still cast shadows,
    // even if their actual depth values fall outside the valid clip space range. This may produce
    // incorrect depth values, but it is still better than not casting shadows at all.
    gl_Position.z = clamp(gl_Position.z, -gl_Position.w, gl_Position.w);
}
