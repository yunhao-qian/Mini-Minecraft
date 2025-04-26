#version 410 core

#include "block_type.glsl"
#include "uniform_buffer_data.glsl"
#include "water_wave.glsl"

uniform float u_waterWaveAmplitudeScale;
uniform int u_cameraIndex;
uniform int u_isAboveWaterOnly;
uniform int u_isUnderWaterOnly;
uniform sampler2DArray u_colorTexture;
uniform sampler2DArray u_normalTexture;

flat in mat3 v_viewSpaceTBNMatrix;
flat in int v_textureIndex;
flat in int v_blockType;
flat in int v_mediumType;
in vec3 v_worldSpacePosition;
in vec2 v_textureCoords;
in float v_waterLevel;

layout(location = 0) out float f_depth;
layout(location = 1) out vec4 f_normal;
layout(location = 2) out vec4 f_albedo;

void main()
{
    {
        bool isAboveWater = v_worldSpacePosition.y >= v_waterLevel;
        if ((u_isAboveWaterOnly != 0 && !isAboveWater)
            || (u_isUnderWaterOnly != 0 && isAboveWater)) {
            discard;
        }
    }
    if (!gl_FrontFacing) {
        if (v_blockType != BlockTypeWater && v_blockType != BlockTypeLava) {
            // The back face of a solid block is not rendered. This is not for performance, but to
            // prevent the back face from being accidentally on the top due to depth precision
            // issues.
            discard;
        }
    }

    mat4 viewMatrix = u_viewMatrices[u_cameraIndex];

    f_depth = -(viewMatrix * vec4(v_worldSpacePosition, 1.0)).z;

    vec3 textureCoords = vec3(v_textureCoords, float(v_textureIndex));
    vec3 viewSpaceNormal = v_viewSpaceTBNMatrix[2];

    {
        vec4 textureNormal = texture(u_normalTexture, textureCoords);
        if (textureNormal.w > 0.5) {
            // The normal map is available for this texture.
            vec3 tangentSpaceNormal = textureNormal.xyz * 2.0 - 1.0;
            // The x component is somehow inverted in the normal map.
            tangentSpaceNormal.x = -tangentSpaceNormal.x;
            f_normal.xyz = normalize(v_viewSpaceTBNMatrix * tangentSpaceNormal);
        } else if (v_blockType == BlockTypeWater) {
            f_normal.xyz = mat3(viewMatrix)
                           * getWaterWaveNormal(v_worldSpacePosition.xz,
                                                u_time,
                                                u_waterWaveAmplitudeScale);
        } else {
            f_normal.xyz = viewSpaceNormal;
        }
    }

    // In the lighting pass, the water surface needs to know whether it is front-facing to compute
    // the correct reflection and refraction vectors. Rather than checking the signs of dot products
    // which may be inaccurate, we store the precise gl_FrontFacing value in the unused w channel of
    // the normal output.
    f_normal.w = gl_FrontFacing ? 1.0 : 0.0;

    f_albedo.rgb = texture(u_colorTexture, textureCoords).rgb;

    // The medium types of the front and back faces may differ, so we determine the actual medium
    // type from the camera's perspective.
    {
        int actualMediumType;
        if (gl_FrontFacing) {
            if (v_mediumType == BlockTypeWater && v_worldSpacePosition.y >= v_waterLevel) {
                // Since the water volume does not occupy the full neighboring block, we must check
                // whether the fragment lies above the water level.
                actualMediumType = BlockTypeAir;
            } else {
                actualMediumType = v_mediumType;
            }
        } else if (v_blockType == BlockTypeWater || v_blockType == BlockTypeLava) {
            actualMediumType = v_blockType;
        } else {
            actualMediumType = BlockTypeAir;
        }

        // Medium types are used in the lighting pass, but a separate texture is unnecessary. We
        // store them in the unused alpha channel of the albedo output.
        f_albedo.a = blockTypeToFloat(actualMediumType);
    }
}
