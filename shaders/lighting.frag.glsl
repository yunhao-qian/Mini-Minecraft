const int NumShadowMapCascades = 4;

uniform mat4 u_viewMatrixInverse;
uniform mat4 u_projectionMatrixInverse;
uniform float u_cameraNear;
uniform float u_cameraFar;
uniform mat4 u_shadowViewMatrices[NumShadowMapCascades];
uniform mat4 u_shadowProjectionMatrices[NumShadowMapCascades];
uniform sampler2DArray u_shadowDepthTexture;
uniform sampler2D u_opaqueNormalTexture;
uniform sampler2D u_opaqueAlbedoTexture;
uniform sampler2D u_opaqueDepthTexture;
uniform sampler2D u_translucentNormalTexture;
uniform sampler2D u_translucentAlbedoTexture;
uniform sampler2D u_translucentDepthTexture;

in vec2 v_textureCoords;

out vec4 f_color;

struct FragmentProperties
{
    int mediumType;
    vec4 albedo;
    float depth;
    float lightIntensity;
};

const vec3 LightDirection = normalize(vec3(1.5, 1.0, 2.0));
const float CascadeBiases[NumShadowMapCascades] = float[](0.004, 0.02, 0.1, 0.5);

FragmentProperties getFragmentProperties(sampler2D normalTexture,
                                         sampler2D albedoTexture,
                                         sampler2D depthTexture)
{
    FragmentProperties properties;

    vec3 normal;
    {
        vec4 normalData = texture(normalTexture, v_textureCoords);
        normal = normalize(normalData.xyz);
        properties.mediumType = blockTypeFromFloat(normalData.a);
    }

    properties.albedo = texture(albedoTexture, v_textureCoords);
    properties.depth = texture(depthTexture, v_textureCoords).r;

    vec4 clipSpacePosition = vec4(v_textureCoords * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpacePosition = u_projectionMatrixInverse * clipSpacePosition;
    viewSpacePosition.xyz *= properties.depth / length(viewSpacePosition.xyz);
    viewSpacePosition.w = 1.0;
    vec4 worldSpacePosition = u_viewMatrixInverse * viewSpacePosition;

    float diffuseTerm = max(dot(normal, LightDirection), 0.0);
    float ambientTerm = 0.2;

    // Compute the cascade index based on the logarithmic split scheme.
    float viewSpaceZ = clamp(viewSpacePosition.z, -u_cameraFar, -u_cameraNear);
    int cascadeIndex = int(floor(float(NumShadowMapCascades) * log(-viewSpaceZ / u_cameraNear)
                                 / log(u_cameraFar / u_cameraNear)));
    cascadeIndex = clamp(cascadeIndex, 0, NumShadowMapCascades - 1);

    vec4 shadowViewSpacePosition = u_shadowViewMatrices[cascadeIndex] * worldSpacePosition;
    vec4 shadowClipSpacePosition = u_shadowProjectionMatrices[cascadeIndex]
                                   * shadowViewSpacePosition;
    vec3 shadowScreenSpacePosition = shadowClipSpacePosition.xyz * (0.5 / shadowClipSpacePosition.w)
                                     + 0.5;
    float shadowDepth
        = texture(u_shadowDepthTexture, vec3(shadowScreenSpacePosition.xy, float(cascadeIndex))).r;

    if (all(greaterThanEqual(shadowScreenSpacePosition, vec3(0.0, 0.0, 0.0)))
        && all(lessThanEqual(shadowScreenSpacePosition, vec3(1.0, 1.0, 1.0))) && shadowDepth != 0.0
        && -shadowViewSpacePosition.z >= shadowDepth + CascadeBiases[cascadeIndex]) {
        diffuseTerm = 0.0;
    }

    properties.lightIntensity = diffuseTerm + ambientTerm;

    return properties;
}

const vec3 SkyColor = vec3(0.37, 0.74, 1.0);

vec3 getAttenuatedColor(vec3 color, float depth, int mediumType)
{
    vec3 attenuationRate;
    vec3 environmentColor;
    if (mediumType == BlockTypeWater) {
        color *= vec3(0.6, 0.8, 1.0);
        attenuationRate = vec3(0.12, 0.08, 0.04);
        environmentColor = vec3(0.15, 0.2, 0.25);
    } else if (mediumType == BlockTypeLava) {
        color *= vec3(0.6, 0.3, 0.0);
        attenuationRate = vec3(0.04, 0.16, 0.24);
        environmentColor = vec3(1.0, 0.4, 0.0);
    } else {
        attenuationRate = vec3(0.002, 0.002, 0.002);
        environmentColor = SkyColor;
    }
    return mix(environmentColor, color, clamp(exp(-depth * attenuationRate), 0.0, 1.0));
}

void main()
{
    FragmentProperties opaqueProperties = getFragmentProperties(u_opaqueNormalTexture,
                                                                u_opaqueAlbedoTexture,
                                                                u_opaqueDepthTexture);
    FragmentProperties translucentProperties = getFragmentProperties(u_translucentNormalTexture,
                                                                     u_translucentAlbedoTexture,
                                                                     u_translucentDepthTexture);

    float farDepth = opaqueProperties.depth;
    vec3 farColor;
    int farMediumType;
    if (opaqueProperties.depth == 0.0) {
        farColor = SkyColor;
        farMediumType = BlockTypeAir;
    } else {
        farColor = opaqueProperties.albedo.rgb * opaqueProperties.lightIntensity;
        farMediumType = opaqueProperties.mediumType;
    }

    float nearDepth;
    vec4 nearColor;
    int nearMediumType;
    if (translucentProperties.depth >= opaqueProperties.depth) {
        nearDepth = 0.0;
        nearColor = vec4(0.0, 0.0, 0.0, 0.0);
        nearMediumType = BlockTypeAir;
    } else {
        nearDepth = translucentProperties.depth;
        nearColor = translucentProperties.albedo * translucentProperties.lightIntensity;
        nearMediumType = translucentProperties.mediumType;
    }

    vec3 finalColor = getAttenuatedColor(farColor, farDepth - nearDepth, farMediumType);
    finalColor = mix(finalColor, nearColor.rgb, nearColor.a);
    finalColor = getAttenuatedColor(finalColor, nearDepth, nearMediumType);
    f_color = vec4(finalColor, 1.0);
}
