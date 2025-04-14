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

vec3 linearColorFromSRGB(vec3 color)
{
    return pow(color, vec3(2.2));
}

vec3 linearColorToSRGB(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

const vec3 SunDirection = normalize(vec3(1.5, 1.0, 2.0));

vec3 getDirectionalLightColor(vec3 direction)
{
    float intensity = max(dot(direction, SunDirection), 0.0);
    return vec3(1.0, 1.0, 1.0) * intensity;
}

struct FragmentProperties
{
    int mediumType;
    float depth;
    vec4 color;
};

FragmentProperties getFragmentProperties(sampler2D normalTexture,
                                         sampler2D albedoTexture,
                                         sampler2D depthTexture,
                                         vec3 viewSpaceDirection,
                                         vec3 worldSpaceDirection)
{
    FragmentProperties properties;
    vec3 normal;
    int blockType;
    {
        vec4 normalData = texture(normalTexture, v_textureCoords);
        normal = normalize(normalData.xyz);
        blockType = blockTypeFromFloat(normalData.w);
    }
    vec3 albedo;
    {
        vec4 albedoData = texture(albedoTexture, v_textureCoords);
        albedo = linearColorFromSRGB(albedoData.rgb);
        properties.mediumType = blockTypeFromFloat(albedoData.a);
    }
    properties.depth = texture(depthTexture, v_textureCoords).r;

    vec4 viewSpacePosition = vec4(viewSpaceDirection * properties.depth, 1.0);
    vec4 worldSpacePosition = u_viewMatrixInverse * viewSpacePosition;

    // Compute the cascade index based on the logarithmic split scheme.
    float viewSpaceZ = clamp(viewSpacePosition.z, -u_cameraFar, -u_cameraNear);
    int cascadeIndex = int(floor(float(ShadowMapCascadeCount) * log(-viewSpaceZ / u_cameraNear)
                                 / log(u_cameraFar / u_cameraNear)));
    cascadeIndex = clamp(cascadeIndex, 0, ShadowMapCascadeCount - 1);

    vec4 shadowViewSpacePosition = u_shadowViewMatrices[cascadeIndex] * worldSpacePosition;
    vec4 shadowClipSpacePosition = u_shadowProjectionMatrices[cascadeIndex]
                                   * shadowViewSpacePosition;
    shadowClipSpacePosition /= shadowClipSpacePosition.w;
    vec2 shadowScreenSpacePosition = shadowClipSpacePosition.xy * 0.5 + 0.5;

    float nonOccludedProbability = 1.0;
    if (all(greaterThanEqual(shadowScreenSpacePosition, vec2(0.0, 0.0)))
        && all(lessThanEqual(shadowScreenSpacePosition, vec2(1.0, 1.0)))) {
        vec4 depthData = texture(u_shadowDepthTexture,
                                 vec3(shadowScreenSpacePosition, float(cascadeIndex)));
        float shadowDepth = depthData.r;
        float shadowDepthSquared = depthData.g;

        float depthVariance = max(shadowDepthSquared - shadowDepth * shadowDepth, 2e-5);
        float depthDifference = max(-shadowViewSpacePosition.z - shadowDepth, 0.0);
        nonOccludedProbability = depthVariance
                                 / (depthVariance + depthDifference * depthDifference);
        // Rescale the probability to reduce light-bleeding artifacts.
        nonOccludedProbability = clamp((nonOccludedProbability - 0.2) / 0.8, 0.0, 1.0);
    }

    vec3 ambientLightColor = vec3(0.2, 0.2, 0.2);
    if (blockType != BlockTypeWater) {
        vec3 lightColor = ambientLightColor
                          + getDirectionalLightColor(normal) * nonOccludedProbability;
        properties.color = vec4(lightColor * albedo, 1.0);
    } else {
        const float WaterN = 1.33; // Refractive index

        // Discard water's texture color as it is unsuitable for the current lighting model.
        albedo = vec3(0.4, 0.6, 0.8);

        float cosTheta = dot(-worldSpaceDirection, normal);
        vec3 lightColor = ambientLightColor;
        float incomingN;
        float outgoingN;
        if (cosTheta < 0.0) {
            // Under water
            normal = -normal;
            cosTheta = -cosTheta;

            lightColor += vec3(0.1, 0.1, 0.1);
            incomingN = WaterN;
            outgoingN = 1.0;
        } else {
            // Above water
            vec3 reflectedDirection = reflect(worldSpaceDirection, normal);
            lightColor += getDirectionalLightColor(reflectedDirection) * nonOccludedProbability;
            incomingN = 1.0;
            outgoingN = WaterN;
        }
        properties.color.rgb = lightColor * albedo;

        // Schlick's approximation:
        // https://en.wikipedia.org/wiki/Schlick%27s_approximation
        float r0 = (incomingN - outgoingN) / (incomingN + outgoingN);
        r0 *= r0;
        float rTheta = r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
        properties.color.a = clamp(rTheta, 0.0, 1.0);
    }

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

    float cosTheta = dot(direction, SunDirection);

    vec3 betaRTheta = 3.0 / (16.0 * Pi) * BetaR * (1.0 + cosTheta * cosTheta);
    vec3 betaMTheta = 1.0 / (4.0 * Pi) * BetaM * (1.0 - G) * (1.0 - G)
                      / pow(1.0 + G * G - 2.0 * G * cosTheta, 1.5);
    vec3 extinctionFactor = exp(-(BetaR + BetaM) * depth);
    vec3 radianceIn = (betaRTheta + betaMTheta) / (BetaR + BetaM) * ESun * (1.0 - extinctionFactor);
    return color * extinctionFactor + radianceIn;
}

vec3 getAttenuatedColor(int mediumType, vec3 color, float depth, vec3 direction)
{
    if (mediumType == BlockTypeWater) {
        color *= vec3(0.8, 0.8, 1.0);
        return getBeerLambertColor(color, depth, vec3(0.20, 0.14, 0.08), vec3(0.06, 0.08, 0.10));
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
    vec4 clipSpacePosition = vec4(v_textureCoords * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpacePosition = u_projectionMatrixInverse * clipSpacePosition;
    vec3 viewSpaceDirection = normalize(viewSpacePosition.xyz);
    vec3 worldSpaceDirection = (u_viewMatrixInverse * vec4(viewSpaceDirection, 0.0)).xyz;

    FragmentProperties opaqueProperties = getFragmentProperties(u_opaqueNormalTexture,
                                                                u_opaqueAlbedoTexture,
                                                                u_opaqueDepthTexture,
                                                                viewSpaceDirection,
                                                                worldSpaceDirection);
    FragmentProperties translucentProperties = getFragmentProperties(u_translucentNormalTexture,
                                                                     u_translucentAlbedoTexture,
                                                                     u_translucentDepthTexture,
                                                                     viewSpaceDirection,
                                                                     worldSpaceDirection);

    int farMediumType;
    vec3 farColor;
    float farDepth;
    if (opaqueProperties.depth < 1e4) {
        farMediumType = opaqueProperties.mediumType;
        farColor = opaqueProperties.color.rgb;
        farDepth = opaqueProperties.depth;
    } else {
        // The sky is not written to in the geometry pass, so we modify the properties manually.
        if (dot(worldSpaceDirection, SunDirection) >= cos(0.02)) {
            // Draw a round disc to simulate the sun. Note that this is much larger than the actual
            // sun.
            farColor = vec3(10.0, 10.0, 10.0);
        } else {
            farColor = vec3(0.0, 0.0, 0.0);
        }
        farColor = vec3(0.0, 0.0, 0.0);
        farMediumType = BlockTypeAir;
        farDepth = 1e4;
    }

    int nearMediumType;
    vec4 nearColor;
    float nearDepth;
    if (translucentProperties.depth >= opaqueProperties.depth) {
        // No translucent fragment is visible.
        nearDepth = 0.0;
        nearColor = vec4(0.0, 0.0, 0.0, 0.0);
        nearMediumType = BlockTypeAir;
    } else {
        nearDepth = translucentProperties.depth;
        nearColor = translucentProperties.color;
        nearMediumType = translucentProperties.mediumType;
    }

    vec3 compositeColor = farColor;
    compositeColor = getAttenuatedColor(farMediumType,
                                        compositeColor,
                                        farDepth - nearDepth,
                                        worldSpaceDirection);
    compositeColor = mix(compositeColor, nearColor.rgb, nearColor.a);
    compositeColor = getAttenuatedColor(nearMediumType,
                                        compositeColor,
                                        nearDepth,
                                        worldSpaceDirection);

    f_color = vec4(linearColorToSRGB(toneMapACES(compositeColor)), 1.0);
}
