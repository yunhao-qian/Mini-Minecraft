uniform mat4 u_shadowViewMatrix;
uniform mat4 u_shadowViewProjectionMatrix;

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
    v_shadowViewSpaceDepth = -(u_shadowViewMatrix * worldSpacePosition).z;
    gl_Position = u_shadowViewProjectionMatrix * worldSpacePosition;
}
