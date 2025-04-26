#version 410 core

#include "block_type.glsl"
#include "uniform_buffer_data.glsl"

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
    const vec2 PoissonDisk[64] = vec2[](vec2(-0.268162, 0.963373),
                                        vec2(0.184829, -0.907099),
                                        vec2(-0.909545, -0.349339),
                                        vec2(0.785118, 0.078684),
                                        vec2(-0.264434, 0.264612),
                                        vec2(0.451911, 0.794221),
                                        vec2(-0.797057, 0.327004),
                                        vec2(-0.281018, -0.412816),
                                        vec2(0.590372, -0.458336),
                                        vec2(-0.016768, -0.053056),
                                        vec2(0.882930, -0.227795),
                                        vec2(0.010505, 0.652915),
                                        vec2(-0.247481, -0.832999),
                                        vec2(0.396323, 0.072193),
                                        vec2(0.236256, -0.283675),
                                        vec2(0.335115, -0.655360),
                                        vec2(-0.630958, -0.581301),
                                        vec2(0.187185, 0.358205),
                                        vec2(-0.691275, -0.037720),
                                        vec2(0.787785, 0.577194),
                                        vec2(-0.503022, 0.487893),
                                        vec2(-0.151739, 0.472888),
                                        vec2(-0.580189, 0.796109),
                                        vec2(-0.481153, -0.240615),
                                        vec2(-0.462645, 0.083268),
                                        vec2(0.598832, 0.389393),
                                        vec2(-0.361799, 0.657831),
                                        vec2(0.536547, -0.164466),
                                        vec2(0.885464, 0.299293),
                                        vec2(0.651634, -0.742260),
                                        vec2(0.233854, 0.962372),
                                        vec2(0.788198, -0.540091),
                                        vec2(-0.038986, -0.790140),
                                        vec2(-0.896057, 0.082593),
                                        vec2(-0.635764, 0.579397),
                                        vec2(-0.672952, -0.217837),
                                        vec2(-0.460501, -0.885826),
                                        vec2(-0.005650, 0.235694),
                                        vec2(-0.239609, -0.202354),
                                        vec2(-0.028256, -0.486447),
                                        vec2(0.188953, -0.006466),
                                        vec2(0.329823, 0.590900),
                                        vec2(-0.821751, 0.510541),
                                        vec2(0.926230, 0.099059),
                                        vec2(0.018844, 0.993394),
                                        vec2(0.432647, -0.855831),
                                        vec2(-0.588625, -0.757227),
                                        vec2(-0.645142, 0.090054),
                                        vec2(0.491468, -0.595039),
                                        vec2(-0.822799, -0.558776),
                                        vec2(-0.476589, -0.506042),
                                        vec2(-0.486940, 0.308349),
                                        vec2(-0.522408, -0.097521),
                                        vec2(0.607207, 0.010414),
                                        vec2(-0.329584, -0.652477),
                                        vec2(0.093296, -0.239199),
                                        vec2(0.507924, 0.609814),
                                        vec2(0.547759, 0.203847),
                                        vec2(0.221560, -0.438634),
                                        vec2(-0.976587, -0.155931),
                                        vec2(0.411172, 0.376810),
                                        vec2(0.976072, -0.091997),
                                        vec2(0.160935, -0.721607),
                                        vec2(0.707924, -0.121700));

    vec3 sampleCoords = vec3(textureCoords, float(cascadeIndex));

    float centerDepth = texture(u_shadowDepthTexture, sampleCoords).r;
    float centerDepthDifference = clamp(-shadowViewSpaceZ - centerDepth, 0.0, 100.0);

    // By estimating the average depth instead of using the center depth directly, we reduce light
    // bleeding artifacts. This is because we want to reduce the blur radius when the sampled point
    // has abrupt depth changes nearby.
    float averageDepth = 0.0;
    float averageDepthSquared = 0.0;
    {
        const int SampleCount = 16;
        vec2 blurRadius = u_shadowMapDepthBlurScales[cascadeIndex].xy * centerDepthDifference;
        for (int i = 0; i < SampleCount; ++i) {
            sampleCoords.xy = textureCoords + PoissonDisk[i] * blurRadius;
            vec4 depthData = texture(u_shadowDepthTexture, sampleCoords);
            averageDepth += depthData.r / float(SampleCount);
            averageDepthSquared += depthData.g / float(SampleCount);
        }
    }
    float averageDepthDifference = clamp(-shadowViewSpaceZ - averageDepth, 0.0, 100.0);

    // Sample the average depth and depth squared in a larger neighborhood.
    DepthMapResult result;
    result.depth = 0.0;
    result.depthSquared = 0.0;
    {
        const int SampleCount = 64;
        // The amount of blurring is proportional to the distance between the shadow-casting object
        // and the shadow receiver.
        vec2 blurRadius = u_shadowMapDepthBlurScales[cascadeIndex].xy * averageDepthDifference;
        for (int i = 0; i < SampleCount; ++i) {
            sampleCoords.xy = textureCoords + PoissonDisk[i] * blurRadius;
            vec4 depthData = texture(u_shadowDepthTexture, sampleCoords);
            float sampleDepth = max(depthData.r, averageDepth);
            float sampleDepthSquared = max(depthData.g, averageDepthSquared);
            result.depth += sampleDepth / float(SampleCount);
            result.depthSquared += sampleDepthSquared / float(SampleCount);
        }
    }
    return result;
}

float getNonOccludedProbability(vec3 viewSpacePosition, float shadowBias)
{
    // Compute the cascade index based on the logarithmic split scheme.
    float viewSpaceZ = clamp(viewSpacePosition.z, -u_cameraFar, -u_cameraNear);
    int cascadeIndex = int(floor(float(ShadowMapCascadeCount) * log(-viewSpaceZ / u_cameraNear)
                                 / log(u_cameraFar / u_cameraNear)));
    cascadeIndex = clamp(cascadeIndex, 0, ShadowMapCascadeCount - 1);

    // These transformations skip the world space, transforming directly from the camera's view
    // space to the shadow map's view and clip spaces.
    vec4 shadowViewSpacePosition = u_mainToShadowViewMatrices[cascadeIndex]
                                   * vec4(viewSpacePosition, 1.0);
    vec4 shadowClipSpacePosition = u_mainToShadowViewProjectionMatrices[cascadeIndex]
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
    float depthDifference = max(-shadowBias - shadowViewSpacePosition.z - result.depth, 0.0);
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
        rayDepth = -viewSpacePosition.z;

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
        rayDepth = -viewSpacePosition.z;
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
                                vec3(0.05, 0.03, 0.02),
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
                                             mat4 viewSpaceConversionMatrix,
                                             float shadowBias)
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
            (u_viewMatrixInverse * vec4(viewSpaceDirection, 0.0)).xyz);
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

        vec3 viewSpacePosition = viewSpaceDirection * (depth / -viewSpaceDirection.z);
        vec3 viewSpaceNormal = normalize(texture(normalTexture, textureCoords).xyz);

        vec3 lightColor = getDirectionalLightColor(viewSpaceNormal, viewSpaceSunDirection);
        lightColor *= getNonOccludedProbability((viewSpaceConversionMatrix
                                                 * vec4(viewSpacePosition, 1.0))
                                                    .xyz,
                                                shadowBias);
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

    vec3 viewSpaceSunDirection = (u_viewMatrices[0] * vec4(WorldSpaceSunDirection, 0.0)).xyz;

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
                                                              mat4(1.0),
                                                              0.0);
    } else {
        // Handle water reflections and refractions.
        // Screen-space reflections and refractions may run into positions not covered by the
        // geometry passes, so we use a very small refractive index to reduce visual artifacts.
        const float EtaWater = 1.1;

        vec3 viewSpaceNormal;
        bool isFrontFacing;
        {
            vec4 normalData = texture(u_translucentNormalTexture, v_textureCoords);
            viewSpaceNormal = normalize(normalData.xyz);
            isFrontFacing = normalData.w > 0.5;
        }
        float cosTheta1 = dot(viewSpaceNormal, -viewSpaceDirection);

        float eta1;
        float eta2;
        if (isFrontFacing) {
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

        float translucentDistance = translucentDepth / -viewSpaceDirection.z;
        vec3 viewSpaceWaterPosition = viewSpaceDirection * translucentDistance;

        vec3 reflectedColor = vec3(0.0);
        {
            vec3 reflectionViewSpaceWaterPosition
                = (u_mainToReflectionViewMatrix * vec4(viewSpaceWaterPosition, 1.0)).xyz;
            vec3 reflectionViewSpaceReflectedDirection
                = (u_mainToReflectionViewMatrix * vec4(viewSpaceReflectedDirection, 0.0)).xyz;
            RayMarchResult result = rayMarch(reflectionViewSpaceWaterPosition,
                                             reflectionViewSpaceReflectedDirection,
                                             u_reflectionProjectionMatrix,
                                             u_reflectionDepthTexture);
            if (result.isHit) {
                vec3 reflectionViewSpaceSunDirection
                    = (u_mainToReflectionViewMatrix * vec4(viewSpaceSunDirection, 0.0)).xyz;
                // Positions computed from screen-space ray marching suffer from precision issues,
                // so we introduce a bias to avoid self-shadowing. Note that this bias is
                // unnecessary for direct-lighted fragments.
                reflectedColor
                    = getOpaqueFragmentColorWithMediumEffects(result.depth,
                                                              result.viewSpaceDirection,
                                                              result.textureCoords,
                                                              reflectionViewSpaceWaterPosition,
                                                              reflectionViewSpaceSunDirection,
                                                              u_reflectionAlbedoTexture,
                                                              u_reflectionNormalTexture,
                                                              u_reflectionToMainViewMatrix,
                                                              0.1);
            }
        }

        vec3 refractedColor = vec3(0.0);
        if (!isFullReflection) {
            // Not full reflection
            vec3 refractionViewSpaceWaterPosition
                = (u_mainToRefractionViewMatrix * vec4(viewSpaceWaterPosition, 1.0)).xyz;
            vec3 refractionViewSpaceRefractedDirection
                = (u_mainToRefractionViewMatrix * vec4(viewSpaceRefractedDirection, 0.0)).xyz;
            RayMarchResult result = rayMarch(refractionViewSpaceWaterPosition,
                                             refractionViewSpaceRefractedDirection,
                                             u_refractionProjectionMatrix,
                                             u_refractionDepthTexture);
            if (result.isHit) {
                vec3 refractionViewSpaceSunDirection
                    = (u_mainToRefractionViewMatrix * vec4(viewSpaceSunDirection, 0.0)).xyz;
                refractedColor
                    = getOpaqueFragmentColorWithMediumEffects(result.depth,
                                                              result.viewSpaceDirection,
                                                              result.textureCoords,
                                                              refractionViewSpaceWaterPosition,
                                                              refractionViewSpaceSunDirection,
                                                              u_refractionAlbedoTexture,
                                                              u_refractionNormalTexture,
                                                              u_refractionToMainViewMatrix,
                                                              0.1);
            }
        }

        vec3 surfaceColor = mix(refractedColor, reflectedColor, reflectionCoefficient);

        int mediumType = blockTypeFromFloat(texture(u_translucentAlbedoTexture, v_textureCoords).a);
        f_color.rgb = applyMediumEffects(mediumType,
                                         surfaceColor,
                                         translucentDistance,
                                         viewSpaceDirection,
                                         viewSpaceSunDirection);
    }

    f_color.rgb = linearColorToSRGB(toneMapACES(f_color.rgb));
    f_color.a = 1.0;
}
