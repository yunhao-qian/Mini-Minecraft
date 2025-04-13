uniform float u_time;
uniform vec3 u_cameraPosition;
uniform sampler2DArray u_colorTexture;
uniform sampler2DArray u_normalTexture;

in vec3 v_worldSpacePosition;
flat in int v_textureIndex;
in vec2 v_textureCoords;
flat in vec3 v_tangent;
flat in vec3 v_bitangent;
flat in vec3 v_normal;
flat in int v_blockType;
flat in int v_mediumType;
in float v_waterElevation;

layout(location = 0) out float f_depth;
layout(location = 1) out vec4 f_normal;
layout(location = 2) out vec4 f_albedo;

void main()
{
    f_depth = distance(u_cameraPosition, v_worldSpacePosition);

    vec3 textureCoords = vec3(v_textureCoords, float(v_textureIndex));

    vec4 textureNormal = texture(u_normalTexture, textureCoords);
    if (textureNormal.a > 0.5) {
        // The normal map is available for this texture.
        mat3 tbnMatrix = mat3(v_tangent, v_bitangent, v_normal);
        vec3 tangentSpaceNormal = textureNormal.xyz * 2.0 - 1.0;
        // The x component is somehow inverted in the normal map.
        tangentSpaceNormal.x = -tangentSpaceNormal.x;
        f_normal = vec4(normalize(tbnMatrix * tangentSpaceNormal), 1.0);
    } else if (v_blockType == BlockTypeWater) {
        f_normal = vec4(getWaterWaveNormal(v_worldSpacePosition.xz, u_time), 1.0);
    } else {
        f_normal = vec4(v_normal, 1.0);
    }

    f_albedo = texture(u_colorTexture, textureCoords);

    // The medium types of the front and back faces may differ, so we determine the actual medium
    // type from the camera's perspective.
    bool isFrontFace = dot(v_normal, u_cameraPosition - v_worldSpacePosition) > 0.0;
    int actualMediumType;
    if (isFrontFace) {
        if (v_mediumType == BlockTypeWater && v_worldSpacePosition.y > v_waterElevation) {
            actualMediumType = BlockTypeAir;
        } else {
            actualMediumType = v_mediumType;
        }
    } else if (v_blockType == BlockTypeWater || v_blockType == BlockTypeLava) {
        actualMediumType = v_blockType;
    } else {
        actualMediumType = BlockTypeAir;
    }

    // The medium type is used in the lighting pass, but it is not worth allocating an additional
    // texture. Instead, we store it in the unused alpha channel of the normal output.
    f_normal.a = blockTypeToFloat(actualMediumType);
}
