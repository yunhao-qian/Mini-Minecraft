uniform mat4 u_viewProjectionMatrix;
uniform float u_time;

layout(location = 0) in ivec3 a_blockPosition;
layout(location = 1) in int a_faceIndex;
layout(location = 2) in int a_textureIndex;
layout(location = 3) in int a_blockType;
layout(location = 4) in int a_mediumType;

out vec3 v_worldPosition;
flat out int v_textureIndex;
out vec2 v_textureCoords;
flat out vec3 v_tangent;
flat out vec3 v_bitangent;
flat out vec3 v_normal;
flat out int v_blockType;
flat out int v_mediumType;

const ivec2 FaceTextureCoords[4] = ivec2[](ivec2(0, 0), ivec2(1, 0), ivec2(1, 1), ivec2(0, 1));

const ivec3 FaceOrigins[6] = ivec3[](ivec3(1, 0, 1),  // Positive X
                                     ivec3(0, 0, 0),  // Negative X
                                     ivec3(0, 1, 1),  // Positive Y
                                     ivec3(0, 0, 0),  // Negative Y
                                     ivec3(0, 0, 1),  // Positive Z
                                     ivec3(1, 0, 0)); // Negative Z

const ivec3 FaceTangents[6] = ivec3[](ivec3(0, 0, -1),
                                      ivec3(0, 0, 1),
                                      ivec3(1, 0, 0),
                                      ivec3(1, 0, 0),
                                      ivec3(1, 0, 0),
                                      ivec3(-1, 0, 0));

const ivec3 FaceBitangents[6] = ivec3[](ivec3(0, 1, 0),
                                        ivec3(0, 1, 0),
                                        ivec3(0, 0, -1),
                                        ivec3(0, 0, 1),
                                        ivec3(0, 1, 0),
                                        ivec3(0, 1, 0));

const vec3 FaceNormals[6] = vec3[](vec3(1.0, 0.0, 0.0),
                                   vec3(-1.0, 0.0, 0.0),
                                   vec3(0.0, 1.0, 0.0),
                                   vec3(0.0, -1.0, 0.0),
                                   vec3(0.0, 0.0, 1.0),
                                   vec3(0.0, 0.0, -1.0));

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

    v_worldPosition = vec3(faceOrigin + textureCoords.x * faceTangent
                           + textureCoords.y * faceBitangent);
    gl_Position = u_viewProjectionMatrix * vec4(v_worldPosition, 1.0);

    v_textureIndex = a_textureIndex;
    v_textureCoords = vec2(textureCoords);
    if (a_blockType == BlockTypeWater) {
        v_textureCoords += randomOffset(8.0);
    } else if (a_blockType == BlockTypeLava) {
        v_textureCoords += randomOffset(4.0);
    }

    v_tangent = vec3(faceTangent);
    v_bitangent = vec3(faceBitangent);
    v_normal = FaceNormals[a_faceIndex];

    v_blockType = a_blockType;
    v_mediumType = a_mediumType;
}
