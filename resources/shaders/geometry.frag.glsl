#version 410 core

#include "block_type.glsl"
#include "uniform_buffer_data.glsl"
#include "water_wave.glsl"

uniform int u_cameraIndex;
uniform int u_isAboveWaterOnly;
uniform int u_isUnderWaterOnly;
uniform sampler2DArray u_colorTexture;
uniform sampler2DArray u_normalTexture;

in vec3 v_worldSpacePosition;
flat in int v_textureIndex;
in vec2 v_textureCoords;
flat in vec3 v_viewSpaceTangent;
flat in vec3 v_viewSpaceBitangent;
flat in vec3 v_viewSpaceNormal;
flat in int v_blockType;
flat in int v_mediumType;
in float v_waterLevel;

layout(location = 0) out float f_depth;
layout(location = 1) out vec4 f_normal;
layout(location = 2) out vec4 f_albedo;

void main()
{
    if (u_isAboveWaterOnly != 0 && v_worldSpacePosition.y < v_waterLevel) {
        discard;
    }
    if (u_isUnderWaterOnly != 0 && v_worldSpacePosition.y > v_waterLevel) {
        discard;
    }

    mat4 viewMatrix = u_viewMatrices[u_cameraIndex];
    f_depth = length((viewMatrix * vec4(v_worldSpacePosition, 1.0)).xyz);

    vec3 textureCoords = vec3(v_textureCoords, float(v_textureIndex));

    vec4 textureNormal = texture(u_normalTexture, textureCoords);
    if (textureNormal.w > 0.5) {
        // The normal map is available for this texture.
        mat3 viewSpaceTBNMatrix = mat3(v_viewSpaceTangent, v_viewSpaceBitangent, v_viewSpaceNormal);
        vec3 tangentSpaceNormal = textureNormal.xyz * 2.0 - 1.0;
        // The x component is somehow inverted in the normal map.
        tangentSpaceNormal.x = -tangentSpaceNormal.x;
        f_normal.xyz = normalize(viewSpaceTBNMatrix * tangentSpaceNormal);
    } else if (v_blockType == BlockTypeWater) {
        f_normal.xyz = (viewMatrix * vec4(getWaterWaveNormal(v_worldSpacePosition.xz, u_time), 0.0))
                           .xyz;
    } else {
        f_normal.xyz = v_viewSpaceNormal;
    }

    f_albedo.rgb = texture(u_colorTexture, textureCoords).rgb;

    // The medium types of the front and back faces may differ, so we determine the actual medium
    // type from the camera's perspective.
    int actualMediumType;
    if (gl_FrontFacing) {
        if (v_mediumType == BlockTypeWater && v_worldSpacePosition.y > v_waterLevel) {
            actualMediumType = BlockTypeAir;
        } else {
            actualMediumType = v_mediumType;
        }
    } else if (v_blockType == BlockTypeWater || v_blockType == BlockTypeLava) {
        actualMediumType = v_blockType;
    } else {
        actualMediumType = BlockTypeAir;
    }

    // The block and medium types are used in the lighting pass, but they are not worth allocating
    // additional textures. Instead, we store them in the unused w/alpha channels of the normal and
    // albedo outputs.
    f_normal.w = blockTypeToFloat(v_blockType);
    f_albedo.a = blockTypeToFloat(actualMediumType);
}
