#version 410 core

#include "block_type.glsl"

const int ShadowMapCascadeCount = 4;

uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrixInverse;
uniform mat4 u_reflectionProjectionMatrix;
uniform mat4 u_originalToReflectionViewMatrix;
uniform mat4 u_reflectionToOriginalViewMatrix;
uniform mat4 u_refractionProjectionMatrix;
uniform mat4 u_originalToRefractionViewMatrix;
uniform mat4 u_refractionToOriginalViewMatrix;
uniform float u_cameraNear;
uniform float u_cameraFar;
uniform mat4 u_shadowViewMatrices[ShadowMapCascadeCount];
uniform mat4 u_shadowViewProjectionMatrices[ShadowMapCascadeCount];
uniform sampler2DArray u_shadowDepthTexture;
uniform sampler2D u_opaqueDepthTexture;
uniform sampler2D u_opaqueNormalTexture;
uniform sampler2D u_opaqueAlbedoTexture;
uniform sampler2D u_translucentDepthTexture;
uniform sampler2D u_translucentNormalTexture;
uniform sampler2D u_translucentAlbedoTexture;
uniform sampler2D u_reflectionDepthTexture;
uniform sampler2D u_reflectionNormalTexture;
uniform sampler2D u_reflectionAlbedoTexture;
uniform sampler2D u_refractionDepthTexture;
uniform sampler2D u_refractionNormalTexture;
uniform sampler2D u_refractionAlbedoTexture;

in vec2 v_textureCoords;

out vec4 f_color;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Color Conversion
////////////////////////////////////////////////////////////////////////////////////////////////////

vec3 linearColorFromSRGB(vec3 color)
{
    return pow(color, vec3(2.2));
}

vec3 linearColorToSRGB(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Shadow Mapping
////////////////////////////////////////////////////////////////////////////////////////////////////

struct DepthMapResult
{
    float depth;
    float depthSquared;
};

DepthMapResult sampleDepthMap(vec2 textureCoords, int cascadeIndex, float shadowViewSpaceZ)
{
    vec3 sampleCoords = vec3(textureCoords, float(cascadeIndex));
    DepthMapResult result;
    vec4 depthData = texture(u_shadowDepthTexture, sampleCoords);
    result.depth = depthData.r;
    result.depthSquared = depthData.g;
    return result;
}

float getNonOccludedProbability(vec3 viewSpacePosition)
{
    // Compute the cascade index based on the logarithmic split scheme.
    float viewSpaceZ = clamp(viewSpacePosition.z, -u_cameraFar, -u_cameraNear);
    int cascadeIndex = int(floor(float(ShadowMapCascadeCount) * log(-viewSpaceZ / u_cameraNear)
                                 / log(u_cameraFar / u_cameraNear)));
    cascadeIndex = clamp(cascadeIndex, 0, ShadowMapCascadeCount - 1);

    // These transformations skip the world space, transforming directly from the camera's view
    // space to the shadow map's view and clip spaces.
    vec4 shadowViewSpacePosition = u_shadowViewMatrices[cascadeIndex]
                                   * vec4(viewSpacePosition, 1.0);
    vec4 shadowClipSpacePosition = u_shadowViewProjectionMatrices[cascadeIndex]
                                   * vec4(viewSpacePosition, 1.0);
    shadowClipSpacePosition /= shadowClipSpacePosition.w;
    vec2 shadowTextureCoords = shadowClipSpacePosition.xy * 0.5 + 0.5;

    if (any(lessThan(shadowTextureCoords, vec2(0.0)))
        || any(greaterThan(shadowTextureCoords, vec2(1.0)))) {
        return 1.0;
    }

    DepthMapResult result = sampleDepthMap(shadowTextureCoords,
                                           cascadeIndex,
                                           shadowViewSpacePosition.z);
    float depthVariance = max(result.depthSquared - result.depth * result.depth, 2e-5);
    float depthDifference = max(-shadowViewSpacePosition.z - result.depth, 0.0);
    float probability = depthVariance / (depthVariance + depthDifference * depthDifference);
    // Rescale the probability to reduce light-bleeding artifacts.
    probability = clamp((probability - 0.2) / 0.8, 0.0, 1.0);
    return probability;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Ray Marching
////////////////////////////////////////////////////////////////////////////////////////////////////

struct RayMarchResult
{
    bool isHit;
    float depth;
    vec3 viewSpaceDirection;
    vec2 textureCoords;
};

RayMarchResult rayMarch(vec3 fromViewSpacePosition,
                        vec3 viewSpaceDirection,
                        mat4 projectionMatrix,
                        sampler2D depthTexture)
{
    bool hasStepSizeChanged = false;
    bool hasHitOccurred = false;

    float stepSize = 0.5;
    float marchDistance = 0.0;
    vec3 viewSpacePosition = fromViewSpacePosition;

    float geometryDepth;
    float rayDepth;
    vec2 textureCoords;

    for (int i = 0; i < 100; ++i) {
        vec4 clipSpacePosition = projectionMatrix * vec4(viewSpacePosition, 1.0);
        clipSpacePosition /= clipSpacePosition.w;
        textureCoords = clipSpacePosition.xy * 0.5 + 0.5;
        if (any(lessThan(textureCoords, vec2(0.0))) || any(greaterThan(textureCoords, vec2(1.0)))) {
            // The ray is out of bounds. Try to step back a bit.
            hasStepSizeChanged = true;
            stepSize = -abs(stepSize);
            marchDistance += stepSize;
            marchDistance = max(marchDistance, 0.0);
            viewSpacePosition = fromViewSpacePosition + viewSpaceDirection * marchDistance;
            continue;
        }

        geometryDepth = texture(depthTexture, textureCoords).r;
        rayDepth = length(viewSpacePosition);

        if (geometryDepth < rayDepth) {
            // Step back.
            hasHitOccurred = true;
            if (stepSize > 0.0) {
                hasStepSizeChanged = true;
                stepSize *= -0.5;
            }
        } else if (geometryDepth > rayDepth) {
            // Step forward.
            if (stepSize < 0.0) {
                hasStepSizeChanged = true;
                stepSize *= -0.5;
            }
        } else {
            // The ray hits a geometry exactly. This is very unlikely, but we handle this case as a
            // theoretical possibility.
            return RayMarchResult(true, rayDepth, normalize(viewSpacePosition), textureCoords);
        }

        marchDistance += stepSize;
        marchDistance = max(marchDistance, 0.0);
        viewSpacePosition = fromViewSpacePosition + viewSpaceDirection * marchDistance;
    }

    if (!hasHitOccurred) {
        if (!hasStepSizeChanged) {
            // The ray goes into the sky.
            return RayMarchResult(true, 1e5, viewSpaceDirection, vec2(0.0));
        }
        // The ray goes out of bounds, and we are unsure if it is a hit or not.
        return RayMarchResult(false, 0.0, vec3(0.0), vec2(0.0));
    }

    // Step back so that the ray is outside the geometry.
    stepSize = -abs(stepSize);
    for (int i = 0; geometryDepth < rayDepth && i < 10; ++i) {
        marchDistance += stepSize;
        marchDistance = max(marchDistance, 0.0);
        viewSpacePosition = fromViewSpacePosition + viewSpaceDirection * marchDistance;
        vec4 clipSpacePosition = projectionMatrix * vec4(viewSpacePosition, 1.0);
        clipSpacePosition /= clipSpacePosition.w;
        textureCoords = clipSpacePosition.xy * 0.5 + 0.5;

        geometryDepth = texture(depthTexture, textureCoords).r;
        rayDepth = length(viewSpacePosition);
    }
    return RayMarchResult(hasHitOccurred, rayDepth, normalize(viewSpacePosition), textureCoords);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Medium Effects
////////////////////////////////////////////////////////////////////////////////////////////////////

vec3 applyBeerLambert(vec3 surfaceColor,
                      float pathLength,
                      vec3 absorptionCoefficient,
                      vec3 scatteredLight)
{
    vec3 transmittance = clamp(exp(-absorptionCoefficient * pathLength), 0.0, 1.0);
    return mix(scatteredLight, surfaceColor, transmittance);
}

vec3 applyAtmosphericScattering(vec3 surfaceColor,
                                float pathLength,
                                vec3 pathDirection,
                                vec3 sunDirection)
{
    // Rendering outdoor light scattering in real time:
    // https://drivers.amd.com/developer/gdc/GDC02_HoffmanPreetham.pdf

    const float Pi = 3.14159265358979323846;
    const vec3 BetaR = vec3(5.8e-6, 13.5e-6, 33.1e-6);
    const vec3 BetaM = vec3(21e-6);
    const float G = 0.76;
    const vec3 ESun = vec3(6.0);

    float cosTheta = dot(pathDirection, sunDirection);

    vec3 betaRTheta = 3.0 / (16.0 * Pi) * BetaR * (1.0 + cosTheta * cosTheta);
    vec3 betaMTheta = 1.0 / (4.0 * Pi) * BetaM * (1.0 - G) * (1.0 - G)
                      / pow(1.0 + G * G - 2.0 * G * cosTheta, 1.5);
    vec3 extinctionFactor = exp(-(BetaR + BetaM) * pathLength);
    vec3 radianceIn = (betaRTheta + betaMTheta) / (BetaR + BetaM) * ESun * (1.0 - extinctionFactor);
    return surfaceColor * extinctionFactor + radianceIn;
}

vec3 applyMediumEffects(
    int mediumType, vec3 surfaceColor, float pathLength, vec3 pathDirection, vec3 sunDirection)
{
    if (mediumType == BlockTypeWater) {
        surfaceColor *= vec3(0.8, 0.8, 1.0);
        return applyBeerLambert(surfaceColor,
                                pathLength,
                                vec3(0.10, 0.07, 0.04),
                                vec3(0.06, 0.08, 0.10));
    }
    if (mediumType == BlockTypeLava) {
        surfaceColor *= vec3(0.6, 0.4, 0.2);
        return applyBeerLambert(surfaceColor,
                                pathLength,
                                vec3(0.01, 0.04, 0.06),
                                vec3(1.0, 0.2, 0.0));
    }
    return applyAtmosphericScattering(surfaceColor, pathLength, pathDirection, sunDirection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Lighting
////////////////////////////////////////////////////////////////////////////////////////////////////

vec3 getDirectionalLightColor(vec3 direction, vec3 sunDirection)
{
    float intensity = max(dot(direction, sunDirection), 0.0);
    return vec3(1.0) * intensity;
}

vec3 getOpaqueFragmentColorWithMediumEffects(float depth,
                                             vec3 viewSpaceDirection,
                                             vec2 textureCoords,
                                             vec3 fromViewSpacePosition,
                                             vec3 viewSpaceSunDirection,
                                             sampler2D albedoTexture,
                                             sampler2D normalTexture,
                                             mat4 viewSpaceConversionMatrix)
{
    int mediumType;
    vec3 surfaceColor;
    float pathLength;
    vec3 viewSpacePathDirection;
    if (depth >= 1e4) {
        // Sky
        mediumType = BlockTypeAir;
        if (dot(viewSpaceDirection, viewSpaceSunDirection) >= cos(0.02)) {
            // Draw a round disc to simulate the sun. Note that this is much larger than the actual
            // sun.
            surfaceColor = vec3(10.0);
        } else {
            surfaceColor = vec3(0.0);
        }
        vec3 worldSpaceDirection = normalize(
            (inverse(u_viewMatrix) * vec4(viewSpaceDirection, 0.0)).xyz);
        const float EarthRadius = 6378e3;
        const float AtmosphereRadius = EarthRadius + 10e3;
        float cosGamma = dot(worldSpaceDirection, vec3(0.0, 1.0, 0.0));
        pathLength = sqrt(AtmosphereRadius * AtmosphereRadius
                          - (1.0 - cosGamma * cosGamma) * EarthRadius * EarthRadius)
                     - EarthRadius * cosGamma;
        viewSpacePathDirection = viewSpaceDirection;
    } else {
        vec3 albedo;
        {
            vec4 albedoData = texture(albedoTexture, textureCoords);
            albedo = linearColorFromSRGB(albedoData.rgb);
            mediumType = blockTypeFromFloat(albedoData.a);
        }

        vec3 viewSpacePosition = viewSpaceDirection * depth;
        vec3 viewSpaceNormal = normalize(texture(normalTexture, textureCoords).xyz);

        vec3 lightColor = getDirectionalLightColor(viewSpaceNormal, viewSpaceSunDirection);
        lightColor *= getNonOccludedProbability(
            (viewSpaceConversionMatrix * vec4(viewSpacePosition, 1.0)).xyz);
        lightColor += 0.2; // Ambient light
        surfaceColor = lightColor * albedo;

        vec3 viewSpacePath = viewSpacePosition - fromViewSpacePosition;
        pathLength = length(viewSpacePath);
        viewSpacePathDirection = normalize(viewSpacePath);
    }

    return applyMediumEffects(mediumType,
                              surfaceColor,
                              pathLength,
                              viewSpacePathDirection,
                              viewSpaceSunDirection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Function
////////////////////////////////////////////////////////////////////////////////////////////////////

void main()
{
    const vec3 WorldSpaceSunDirection = normalize(vec3(1.5, 1.0, 2.0));

    vec3 viewSpaceSunDirection = (u_viewMatrix * vec4(WorldSpaceSunDirection, 0.0)).xyz;

    float opaqueDepth = texture(u_opaqueDepthTexture, v_textureCoords).r;
    float translucentDepth = texture(u_translucentDepthTexture, v_textureCoords).r;

    vec4 clipSpacePosition = vec4(v_textureCoords * 2.0 - 1.0, 1.0, 1.0);
    vec3 viewSpaceDirection = normalize((u_projectionMatrixInverse * clipSpacePosition).xyz);

    if (translucentDepth >= opaqueDepth) {
        // No translucent (water) fragment is visible.
        f_color.rgb = getOpaqueFragmentColorWithMediumEffects(opaqueDepth,
                                                              viewSpaceDirection,
                                                              v_textureCoords,
                                                              vec3(0.0),
                                                              viewSpaceSunDirection,
                                                              u_opaqueAlbedoTexture,
                                                              u_opaqueNormalTexture,
                                                              mat4(1.0));
    } else {
        // Handle water reflections and refractions.
        // Screen-space reflections and refractions may run into positions not covered by the
        // geometry passes, so we use a very small refractive index to reduce visual artifacts.
        const float EtaWater = 1.1;

        vec3 viewSpaceNormal = normalize(texture(u_translucentNormalTexture, v_textureCoords).xyz);
        float cosTheta1 = dot(viewSpaceNormal, -viewSpaceDirection);

        float eta1;
        float eta2;
        if (cosTheta1 >= 0.0) {
            eta1 = 1.0;
            eta2 = EtaWater;
        } else {
            viewSpaceNormal = -viewSpaceNormal;
            cosTheta1 = -cosTheta1;
            eta1 = EtaWater;
            eta2 = 1.0;
        }

        vec3 viewSpaceReflectedDirection = reflect(viewSpaceDirection, viewSpaceNormal);
        vec3 viewSpaceRefractedDirection = refract(viewSpaceDirection, viewSpaceNormal, eta1 / eta2);
        bool isFullReflection = length(viewSpaceRefractedDirection) < 1e-3;

        float reflectionCoefficient;
        if (isFullReflection) {
            reflectionCoefficient = 1.0;
        } else {
            float cosTheta2 = dot(-viewSpaceNormal, viewSpaceRefractedDirection);

            float eta2CosTheta1 = eta2 * cosTheta1;
            float eta1CosTheta2 = eta1 * cosTheta2;
            float fs = (eta2CosTheta1 - eta1CosTheta2) / (eta2CosTheta1 + eta1CosTheta2);

            float eta1CosTheta1 = eta1 * cosTheta1;
            float eta2CosTheta2 = eta2 * cosTheta2;
            float fp = (eta1CosTheta1 - eta2CosTheta2) / (eta1CosTheta1 + eta2CosTheta2);

            reflectionCoefficient = 0.5 * (fs * fs + fp * fp);
        }

        vec3 viewSpaceWaterPosition = viewSpaceDirection * translucentDepth;

        vec3 reflectedColor = vec3(0.0);
        {
            vec3 reflectionViewSpaceWaterPosition
                = (u_originalToReflectionViewMatrix * vec4(viewSpaceWaterPosition, 1.0)).xyz;
            vec3 reflectionViewSpaceReflectedDirection
                = (u_originalToReflectionViewMatrix * vec4(viewSpaceReflectedDirection, 0.0)).xyz;
            RayMarchResult result = rayMarch(reflectionViewSpaceWaterPosition,
                                             reflectionViewSpaceReflectedDirection,
                                             u_reflectionProjectionMatrix,
                                             u_reflectionDepthTexture);
            if (result.isHit) {
                vec3 reflectionViewSpaceSunDirection
                    = (u_originalToReflectionViewMatrix * vec4(viewSpaceSunDirection, 0.0)).xyz;
                reflectedColor
                    = getOpaqueFragmentColorWithMediumEffects(result.depth,
                                                              result.viewSpaceDirection,
                                                              result.textureCoords,
                                                              reflectionViewSpaceWaterPosition,
                                                              reflectionViewSpaceSunDirection,
                                                              u_reflectionAlbedoTexture,
                                                              u_reflectionNormalTexture,
                                                              u_reflectionToOriginalViewMatrix);
            }
        }

        vec3 refractedColor = vec3(0.0);
        if (!isFullReflection) {
            // Not full reflection
            vec3 refractionViewSpaceWaterPosition
                = (u_originalToRefractionViewMatrix * vec4(viewSpaceWaterPosition, 1.0)).xyz;
            vec3 refractionViewSpaceRefractedDirection
                = (u_originalToRefractionViewMatrix * vec4(viewSpaceRefractedDirection, 0.0)).xyz;
            RayMarchResult result = rayMarch(refractionViewSpaceWaterPosition,
                                             refractionViewSpaceRefractedDirection,
                                             u_refractionProjectionMatrix,
                                             u_refractionDepthTexture);
            if (result.isHit) {
                vec3 refractionViewSpaceSunDirection
                    = (u_originalToRefractionViewMatrix * vec4(viewSpaceSunDirection, 0.0)).xyz;
                refractedColor
                    = getOpaqueFragmentColorWithMediumEffects(result.depth,
                                                              result.viewSpaceDirection,
                                                              result.textureCoords,
                                                              refractionViewSpaceWaterPosition,
                                                              refractionViewSpaceSunDirection,
                                                              u_refractionAlbedoTexture,
                                                              u_refractionNormalTexture,
                                                              u_refractionToOriginalViewMatrix);
            }
        }

        vec3 surfaceColor = mix(refractedColor, reflectedColor, reflectionCoefficient);

        int mediumType = blockTypeFromFloat(texture(u_translucentAlbedoTexture, v_textureCoords).a);
        f_color.rgb = applyMediumEffects(mediumType,
                                         surfaceColor,
                                         translucentDepth,
                                         viewSpaceDirection,
                                         viewSpaceSunDirection);
    }

    f_color.rgb = linearColorToSRGB(toneMapACES(f_color.rgb));
    f_color.a = 1.0;
}
