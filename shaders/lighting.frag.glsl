const int ShadowMapCascadeCount = 4;

uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_projectionMatrixInverse;
uniform mat4 u_originalToReflectedViewMatrix;
uniform mat4 u_reflectedToOriginalViewMatrix;
uniform float u_cameraNear;
uniform float u_cameraFar;
uniform mat4 u_shadowViewMatrices[ShadowMapCascadeCount];
uniform mat4 u_shadowViewProjectionMatrices[ShadowMapCascadeCount];
uniform vec2 u_shadowMapDepthBlurScales[ShadowMapCascadeCount];
uniform sampler2DArray u_shadowDepthTexture;
uniform sampler2D u_opaqueDepthTexture;
uniform sampler2D u_opaqueNormalTexture;
uniform sampler2D u_opaqueAlbedoTexture;
uniform sampler2D u_translucentDepthTexture;
uniform sampler2D u_translucentNormalTexture;
uniform sampler2D u_translucentAlbedoTexture;
uniform sampler2D u_aboveWaterDepthTexture;
uniform sampler2D u_aboveWaterNormalTexture;
uniform sampler2D u_aboveWaterAlbedoTexture;
uniform sampler2D u_underWaterDepthTexture;
uniform sampler2D u_underWaterNormalTexture;
uniform sampler2D u_underWaterAlbedoTexture;
uniform sampler2D u_reflectedAboveWaterDepthTexture;
uniform sampler2D u_reflectedAboveWaterNormalTexture;
uniform sampler2D u_reflectedAboveWaterAlbedoTexture;
uniform sampler2D u_reflectedUnderWaterDepthTexture;
uniform sampler2D u_reflectedUnderWaterNormalTexture;
uniform sampler2D u_reflectedUnderWaterAlbedoTexture;

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
    vec3 sampleCoords = vec3(0.0, 0.0, float(cascadeIndex));

    // Sample the average depth in a small neighborhood.
    float averageShadowDepth = 0.0;
    {
        const int HalfNumSamples = 1;
        const float Multiplier = 1.0 / float((HalfNumSamples * 2 + 1) * (HalfNumSamples * 2 + 1));

        vec2 halfSize = u_shadowMapDepthBlurScales[cascadeIndex];
        vec2 offsetStep = halfSize / float(HalfNumSamples);

        for (int iy = -HalfNumSamples; iy <= HalfNumSamples; ++iy) {
            sampleCoords.y = textureCoords.y + float(iy) * offsetStep.y;
            for (int ix = -HalfNumSamples; ix <= HalfNumSamples; ++ix) {
                sampleCoords.x = textureCoords.x + float(ix) * offsetStep.x;
                float depth = texture(u_shadowDepthTexture, sampleCoords).r;
                averageShadowDepth += depth * Multiplier;
            }
        }
    }

    // The amount of blurring is proportional to the distance between the shadow-casting object and
    // the shadow receiver.
    float averageDepthDifference = -shadowViewSpaceZ - averageShadowDepth;
    averageDepthDifference = clamp(averageDepthDifference, 5.0, 100.0);

    // Sample the average depth and depth squared in a larger neighborhood.
    DepthMapResult result;
    result.depth = 0.0;
    result.depthSquared = 0.0;
    {
        const int HalfNumSamples = 8;
        const float Multiplier = 1.0 / float((HalfNumSamples * 2 + 1) * (HalfNumSamples * 2 + 1));

        vec2 halfSize = u_shadowMapDepthBlurScales[cascadeIndex] * averageDepthDifference;
        vec2 offsetStep = halfSize / float(HalfNumSamples);

        for (int iy = -HalfNumSamples; iy <= HalfNumSamples; ++iy) {
            sampleCoords.y = textureCoords.y + float(iy) * offsetStep.y;
            for (int ix = -HalfNumSamples; ix <= HalfNumSamples; ++ix) {
                sampleCoords.x = textureCoords.x + float(ix) * offsetStep.x;
                float depth = texture(u_shadowDepthTexture, sampleCoords).r;
                result.depth += depth * Multiplier;
                result.depthSquared += depth * depth * Multiplier;
            }
        }
    }

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

RayMarchResult rayMarch(vec3 fromViewSpacePosition, vec3 viewSpaceDirection, sampler2D depthTexture)
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
        // Both the original and the reflected cameras share the same projection matrix.
        vec4 clipSpacePosition = u_projectionMatrix * vec4(viewSpacePosition, 1.0);
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
        vec4 clipSpacePosition = u_projectionMatrix * vec4(viewSpacePosition, 1.0);
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
                                vec3(0.20, 0.14, 0.08),
                                vec3(0.06, 0.08, 0.10));
    }
    if (mediumType == BlockTypeLava) {
        surfaceColor *= vec3(0.6, 0.4, 0.2);
        return applyBeerLambert(surfaceColor,
                                pathLength,
                                vec3(0.01, 0.04, 0.06),
                                vec3(1.0, 0.2, 0.0));
    }
    // Hack: Scale the path length to make the atmospheric scattering more visible.
    return applyAtmosphericScattering(surfaceColor, pathLength * 10.0, pathDirection, sunDirection);
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
        pathLength = 1e4;
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
        const float WaterRefractiveIndex = 1.02;

        vec3 viewSpaceNormal = normalize(texture(u_translucentNormalTexture, v_textureCoords).xyz);
        float cosTheta = dot(viewSpaceNormal, -viewSpaceDirection);
        bool isAboveWater = cosTheta >= 0.0;

        float incomingRefractiveIndex;
        float outgoingRefractiveIndex;
        if (isAboveWater) {
            incomingRefractiveIndex = 1.0;
            outgoingRefractiveIndex = WaterRefractiveIndex;
        } else {
            viewSpaceNormal = -viewSpaceNormal;
            cosTheta = -cosTheta;
            incomingRefractiveIndex = WaterRefractiveIndex;
            outgoingRefractiveIndex = 1.0;
        }

        vec3 viewSpaceReflectedDirection = reflect(viewSpaceDirection, viewSpaceNormal);
        vec3 viewSpaceRefractedDirection = refract(viewSpaceDirection,
                                                   viewSpaceNormal,
                                                   incomingRefractiveIndex
                                                       / outgoingRefractiveIndex);

        // Schlick's approximation:
        // https://en.wikipedia.org/wiki/Schlick%27s_approximation
        float reflectionCoefficient;
        {
            float r0 = (incomingRefractiveIndex - outgoingRefractiveIndex)
                       / (incomingRefractiveIndex + outgoingRefractiveIndex);
            r0 *= r0;
            reflectionCoefficient = r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
            reflectionCoefficient = clamp(reflectionCoefficient, 0.0, 1.0);
        }

        vec3 viewSpaceWaterPosition = viewSpaceDirection * translucentDepth;

        vec3 reflectedColor = vec3(0.0);
        {
            vec3 reflectedViewSpaceWaterPosition
                = (u_originalToReflectedViewMatrix * vec4(viewSpaceWaterPosition, 1.0)).xyz;
            vec3 reflectedViewSpaceReflectedDirection
                = (u_originalToReflectedViewMatrix * vec4(viewSpaceReflectedDirection, 0.0)).xyz;
            RayMarchResult result = rayMarch(reflectedViewSpaceWaterPosition,
                                             reflectedViewSpaceReflectedDirection,
                                             isAboveWater ? u_reflectedAboveWaterDepthTexture
                                                          : u_reflectedUnderWaterDepthTexture);
            if (result.isHit) {
                vec3 reflectedViewSpaceSunDirection
                    = (u_originalToReflectedViewMatrix * vec4(viewSpaceSunDirection, 0.0)).xyz;
                reflectedColor = getOpaqueFragmentColorWithMediumEffects(
                    result.depth,
                    result.viewSpaceDirection,
                    result.textureCoords,
                    reflectedViewSpaceWaterPosition,
                    reflectedViewSpaceSunDirection,
                    isAboveWater ? u_reflectedAboveWaterAlbedoTexture
                                 : u_reflectedUnderWaterAlbedoTexture,
                    isAboveWater ? u_reflectedAboveWaterNormalTexture
                                 : u_reflectedUnderWaterNormalTexture,
                    u_reflectedToOriginalViewMatrix);
            }
        }

        vec3 refractedColor = vec3(0.0);
        if (length(viewSpaceRefractedDirection) > 1e-3) {
            // Not full reflection
            RayMarchResult result = rayMarch(viewSpaceWaterPosition,
                                             viewSpaceRefractedDirection,
                                             isAboveWater ? u_underWaterDepthTexture
                                                          : u_aboveWaterDepthTexture);
            if (result.isHit) {
                refractedColor = getOpaqueFragmentColorWithMediumEffects(
                    result.depth,
                    result.viewSpaceDirection,
                    result.textureCoords,
                    viewSpaceWaterPosition,
                    viewSpaceSunDirection,
                    isAboveWater ? u_underWaterAlbedoTexture : u_aboveWaterAlbedoTexture,
                    isAboveWater ? u_underWaterNormalTexture : u_aboveWaterNormalTexture,
                    mat4(1.0));
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
