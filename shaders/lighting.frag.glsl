const int ShadowMapCascadeCount = 4;

uniform mat4 u_viewMatrixInverse;
uniform mat4 u_projectionMatrixInverse;
uniform float u_cameraNear;
uniform float u_cameraFar;
uniform mat4 u_shadowViewMatrices[ShadowMapCascadeCount];
uniform mat4 u_shadowProjectionMatrices[ShadowMapCascadeCount];
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
    vec3 direction;
};

vec3 linearColorFromSRGB(vec3 color)
{
    return pow(color, vec3(2.2));
}

vec3 linearColorToSRGB(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

const vec3 LightDirection = normalize(vec3(1.5, 1.0, 2.0));

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
    properties.albedo.rgb = linearColorFromSRGB(properties.albedo.rgb);
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
    int cascadeIndex = int(floor(float(ShadowMapCascadeCount) * log(-viewSpaceZ / u_cameraNear)
                                 / log(u_cameraFar / u_cameraNear)));
    cascadeIndex = clamp(cascadeIndex, 0, ShadowMapCascadeCount - 1);

    vec4 shadowViewSpacePosition = u_shadowViewMatrices[cascadeIndex] * worldSpacePosition;
    vec4 shadowClipSpacePosition = u_shadowProjectionMatrices[cascadeIndex]
                                   * shadowViewSpacePosition;
    vec3 shadowScreenSpacePosition = shadowClipSpacePosition.xyz * (0.5 / shadowClipSpacePosition.w)
                                     + 0.5;

    if (all(greaterThanEqual(shadowScreenSpacePosition, vec3(0.0, 0.0, 0.0)))
        && all(lessThanEqual(shadowScreenSpacePosition, vec3(1.0, 1.0, 1.0)))) {
        vec4 depthData = texture(u_shadowDepthTexture,
                                 vec3(shadowScreenSpacePosition.xy, float(cascadeIndex)));
        float shadowDepth = depthData.r;
        float shadowDepthSquared = depthData.g;

        float depthVariance = max(shadowDepthSquared - shadowDepth * shadowDepth, 2e-5);
        float depthDifference = max(-shadowViewSpacePosition.z - shadowDepth, 0.0);
        float probability = depthVariance / (depthVariance + depthDifference * depthDifference);
        // Rescale the probability to reduce light-bleeding artifacts.
        probability = (probability - 0.2) / 0.8;
        diffuseTerm *= clamp(probability, 0.0, 1.0);
    }

    properties.lightIntensity = diffuseTerm + ambientTerm;

    properties.direction = normalize(vec3(u_viewMatrixInverse * vec4(viewSpacePosition.xyz, 0.0)));

    return properties;
}

vec3 getBeerLambertColor(vec3 color, float depth, vec3 attenuationRate, vec3 environmentColor)
{
    return mix(environmentColor, color, clamp(exp(-depth * attenuationRate), 0.0, 1.0));
}

vec3 getAtmosphericScatteredColor(vec3 color, float depth, vec3 direction)
{
    // Reference:
    // https://drivers.amd.com/developer/gdc/GDC02_HoffmanPreetham.pdf

    const float Pi = 3.14159265358979323846;
    const vec3 BetaR = vec3(5.8e-6, 13.5e-6, 33.1e-6);
    const vec3 BetaM = vec3(21e-6, 21e-6, 21e-6);
    const float G = 0.76;
    const vec3 ESun = vec3(10.0, 10.0, 10.0);

    float cosTheta = dot(direction, LightDirection);

    vec3 betaRTheta = 3.0 / (16.0 * Pi) * BetaR * (1.0 + cosTheta * cosTheta);
    vec3 betaMTheta = 1.0 / (4.0 * Pi) * BetaM * (1.0 - G) * (1.0 - G)
                      / pow(1.0 + G * G - 2.0 * G * cosTheta, 1.5);
    vec3 extinctionFactor = exp(-(BetaR + BetaM) * depth);
    vec3 radianceIn = (betaRTheta + betaMTheta) / (BetaR + BetaM) * ESun * (1.0 - extinctionFactor);
    return color * extinctionFactor + radianceIn;
}

vec3 getAttenuatedColor(int mediumType, vec3 color, float depth, vec3 direction)
{
    vec3 attenuationRate;
    vec3 environmentColor;
    if (mediumType == BlockTypeWater) {
        color *= vec3(0.6, 0.8, 1.0);
        return getBeerLambertColor(color, depth, vec3(0.06, 0.04, 0.01), vec3(0.06, 0.08, 0.12));
    }
    if (mediumType == BlockTypeLava) {
        color *= vec3(0.6, 0.4, 0.2);
        return getBeerLambertColor(color, depth, vec3(0.01, 0.04, 0.06), vec3(1.0, 0.2, 0.0));
    }
    // Hack: Scale the depth to make the atmospheric scattering more visible.
    return getAtmosphericScatteredColor(color, depth * 10.0, direction);
}

vec3 toneMapACES(vec3 color)
{
    // ACES filmic tone mapping curve:
    // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/

    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

void main()
{
    FragmentProperties opaqueProperties = getFragmentProperties(u_opaqueNormalTexture,
                                                                u_opaqueAlbedoTexture,
                                                                u_opaqueDepthTexture);
    FragmentProperties translucentProperties = getFragmentProperties(u_translucentNormalTexture,
                                                                     u_translucentAlbedoTexture,
                                                                     u_translucentDepthTexture);

    int farMediumType;
    vec3 farColor;
    float farDepth;
    if (opaqueProperties.depth >= 1e4) {
        if (dot(opaqueProperties.direction, LightDirection) >= cos(0.02)) {
            // Draw a round disc to simulate the sun. Note that this is much larger than the actual
            // sun.
            farColor = vec3(10.0, 10.0, 10.0);
        } else {
            farColor = vec3(0.0, 0.0, 0.0);
        }
        farColor = vec3(0.0, 0.0, 0.0);
        farMediumType = BlockTypeAir;
        // For sky areas, use this depth for atmospheric scattering.
        farDepth = 1e4;
    } else {
        farColor = opaqueProperties.albedo.rgb * opaqueProperties.lightIntensity;
        farMediumType = opaqueProperties.mediumType;
        farDepth = opaqueProperties.depth;
    }

    int nearMediumType;
    vec4 nearColor;
    float nearDepth;
    if (translucentProperties.depth >= opaqueProperties.depth) {
        nearDepth = 0.0;
        nearColor = vec4(0.0, 0.0, 0.0, 0.0);
        nearMediumType = BlockTypeAir;
    } else {
        nearDepth = translucentProperties.depth;
        nearColor = translucentProperties.albedo * translucentProperties.lightIntensity;
        nearMediumType = translucentProperties.mediumType;
    }

    vec3 compositeColor = getAttenuatedColor(farMediumType,
                                             farColor,
                                             farDepth - nearDepth,
                                             opaqueProperties.direction);
    compositeColor = mix(compositeColor, nearColor.rgb, nearColor.a);
    compositeColor = getAttenuatedColor(nearMediumType,
                                        compositeColor,
                                        nearDepth,
                                        translucentProperties.direction);

    f_color = vec4(linearColorToSRGB(toneMapACES(compositeColor)), 1.0);
}
