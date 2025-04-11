uniform mat4 u_shadowViewProjectionMatrix;

layout(location = 0) in ivec3 a_blockPosition;
layout(location = 1) in int a_faceIndex;

void main()
{
    ivec3 faceOrigin = a_blockPosition + FaceOrigins[a_faceIndex];
    ivec3 faceTangent = FaceTangents[a_faceIndex];
    ivec3 faceBitangent = FaceBitangents[a_faceIndex];
    ivec2 textureCoords = FaceTextureCoords[gl_VertexID];

    vec3 worldSpacePosition = vec3(faceOrigin + textureCoords.x * faceTangent
                                   + textureCoords.y * faceBitangent);
    gl_Position = u_shadowViewProjectionMatrix * vec4(worldSpacePosition, 1.0);
}
