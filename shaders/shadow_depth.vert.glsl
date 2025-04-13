uniform mat4 u_shadowViewMatrix;
uniform mat4 u_shadowProjectionMatrix;

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
    vec4 viewSpacePosition = u_shadowViewMatrix * worldSpacePosition;

    v_shadowViewSpaceDepth = -viewSpacePosition.z;
    // TODO: Calculating the screen-space position in this way causes obstacles outside of the
    // camera's view frustum to not cast shadows, and the shadow effect to be dependent on the
    // camera's orientation. This should be fixed.
    gl_Position = u_shadowProjectionMatrix * viewSpacePosition;
}
