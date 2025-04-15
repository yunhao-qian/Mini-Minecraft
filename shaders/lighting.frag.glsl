const int ShadowMapCascadeCount = 4;

uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_projectionMatrixInverse;
uniform float u_cameraNear;
uniform float u_cameraFar;
uniform mat4 u_shadowViewMatrices[ShadowMapCascadeCount];
uniform mat4 u_shadowViewProjectionMatrices[ShadowMapCascadeCount];
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

vec3 getDirectionalLightColor(vec3 direction, vec3 sunDirection)
{
    float intensity = max(dot(direction, sunDirection), 0.0);
    return vec3(1.0) * intensity;
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

    vec4 depthData = texture(u_shadowDepthTexture, vec3(shadowTextureCoords, float(cascadeIndex)));
    float shadowDepth = depthData.r;
    float shadowDepthSquared = depthData.g;

    float depthVariance = max(shadowDepthSquared - shadowDepth * shadowDepth, 2e-5);
    float depthDifference = max(-shadowViewSpacePosition.z - shadowDepth, 0.0);
    float probability = depthVariance / (depthVariance + depthDifference * depthDifference);
    // Rescale the probability to reduce light-bleeding artifacts.
    probability = clamp((probability - 0.2) / 0.8, 0.0, 1.0);
    return probability;
}

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
    const vec3 ESun = vec3(10.0);

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

vec3 getOpaqueFragmentColorWithMediumEffects(float depth,
                                             vec3 viewSpaceDirection,
                                             vec2 textureCoords,
                                             vec3 fromViewSpacePosition,
                                             vec3 viewSpaceSunDirection)
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
            vec4 albedoData = texture(u_opaqueAlbedoTexture, textureCoords);
            albedo = linearColorFromSRGB(albedoData.rgb);
            mediumType = blockTypeFromFloat(albedoData.a);
        }

        vec3 viewSpacePosition = viewSpaceDirection * depth;
        vec3 viewSpaceNormal = normalize(texture(u_opaqueNormalTexture, textureCoords).xyz);

        vec3 lightColor = getDirectionalLightColor(viewSpaceNormal, viewSpaceSunDirection);
        lightColor *= getNonOccludedProbability(viewSpacePosition);
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
                                                              viewSpaceSunDirection);
    } else {
        // Handle water reflections and refractions.
        // The refraction approximation is very rough, so we pick a very small refractive index to
        // reduce visual artifacts.
        const float WaterRefractiveIndex = 1.02;

        vec3 viewSpaceNormal = normalize(texture(u_translucentNormalTexture, v_textureCoords).xyz);
        float cosTheta = dot(viewSpaceNormal, -viewSpaceDirection);

        float incomingRefractiveIndex;
        float outgoingRefractiveIndex;
        if (cosTheta < 0.0) {
            // Under water
            viewSpaceNormal = -viewSpaceNormal;
            cosTheta = -cosTheta;
            incomingRefractiveIndex = WaterRefractiveIndex;
            outgoingRefractiveIndex = 1.0;
        } else {
            // Above water
            incomingRefractiveIndex = 1.0;
            outgoingRefractiveIndex = WaterRefractiveIndex;
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

        vec3 reflectedColor = getDirectionalLightColor(viewSpaceReflectedDirection,
                                                       viewSpaceSunDirection);
        reflectedColor *= getNonOccludedProbability(viewSpaceWaterPosition);
        reflectedColor += 0.2; // Ambient light

        vec3 refractedColor;
        if (length(viewSpaceRefractedDirection) < 1e-3) {
            // Full reflection
            refractedColor = vec3(0.0);
        } else {
            // This is based the naive assumption that the background depth along the refracted
            // direction is the same as the background depth along the view direction.
            vec3 viewSpaceBackgroundDirection = normalize(viewSpaceWaterPosition
                                                          + viewSpaceRefractedDirection
                                                                * (opaqueDepth - translucentDepth));
            vec4 clipSpaceBackgroundPosition = u_projectionMatrix
                                               * vec4(viewSpaceBackgroundDirection, 1.0);
            clipSpaceBackgroundPosition /= clipSpaceBackgroundPosition.w;
            vec2 backgroundTextureCoords = clipSpaceBackgroundPosition.xy * 0.5 + 0.5;
            float backgroundDepth = texture(u_opaqueDepthTexture, backgroundTextureCoords).r;
            refractedColor = getOpaqueFragmentColorWithMediumEffects(backgroundDepth,
                                                                     viewSpaceBackgroundDirection,
                                                                     backgroundTextureCoords,
                                                                     viewSpaceWaterPosition,
                                                                     viewSpaceSunDirection);
        }

        vec3 surfaceColor = mix(refractedColor, reflectedColor, reflectionCoefficient);
        surfaceColor *= vec3(0.4, 0.6, 0.8);

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
