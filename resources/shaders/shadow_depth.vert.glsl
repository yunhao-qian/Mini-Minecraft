#version 410 core

#include "block_face.glsl"
#include "uniform_buffer_data.glsl"

uniform int u_cascadeIndex;

layout(location = 0) in ivec3 a_faceOrigin;
layout(location = 1) in int a_faceIndex;

out float v_shadowViewSpaceDepth;

void main()
{
    ivec2 textureCoords = FaceTextureCoords[gl_VertexID];

    vec4 worldSpacePosition = vec4(vec3(a_faceOrigin + textureCoords.x * FaceTangents[a_faceIndex]
                                        + textureCoords.y * FaceBitangents[a_faceIndex]),
                                   1.0);

    v_shadowViewSpaceDepth = -(u_shadowViewMatrices[u_cascadeIndex] * worldSpacePosition).z;

    gl_Position = u_shadowViewProjectionMatrices[u_cascadeIndex] * worldSpacePosition;

    // Clamping the depth ensures that geometry outside the shadow frustum can still cast shadows,
    // even if their actual depth values fall outside the valid clip space range. This may produce
    // incorrect depth values, but it is still better than not casting shadows at all.
    gl_Position.z = clamp(gl_Position.z, -gl_Position.w, gl_Position.w);
}
