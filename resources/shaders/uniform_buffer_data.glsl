const int ShadowMapCascadeCount = 6;

layout(std140) uniform UniformBufferData
{
    float u_time;

    float u_cameraNear;
    float u_cameraFar;

    // 0 = main camera
    // 1 = reflection camera
    // 2 = refraction camera
    mat4 u_viewMatrices[3];
    mat4 u_viewProjectionMatrices[3];

    mat4 u_shadowViewMatrices[ShadowMapCascadeCount];
    mat4 u_shadowViewProjectionMatrices[ShadowMapCascadeCount];

    mat4 u_mainToShadowViewMatrices[ShadowMapCascadeCount];
    mat4 u_mainToShadowViewProjectionMatrices[ShadowMapCascadeCount];

    mat4 u_viewMatrixInverse;
    mat4 u_projectionMatrixInverse;

    mat4 u_reflectionProjectionMatrix;
    mat4 u_mainToReflectionViewMatrix;
    mat4 u_reflectionToMainViewMatrix;

    mat4 u_refractionProjectionMatrix;
    mat4 u_mainToRefractionViewMatrix;
    mat4 u_refractionToMainViewMatrix;
};
