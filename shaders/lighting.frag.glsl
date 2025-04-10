uniform vec3 u_cameraPosition;
uniform mat4 u_viewProjectionMatrixInverse;
uniform sampler2D u_opaqueNormalTexture;
uniform sampler2D u_opaqueAlbedoTexture;
uniform sampler2D u_opaqueDepthTexture;
uniform sampler2D u_translucentNormalTexture;
uniform sampler2D u_translucentAlbedoTexture;
uniform sampler2D u_translucentDepthTexture;

in vec2 v_textureCoords;

out vec4 f_color;

struct FragmentAttributes
{
    vec3 normal;
    int mediumType;
    vec4 albedo;
    float screenSpaceDepth;
    vec3 worldPosition;
    float worldSpaceDepth;
};

FragmentAttributes getFragmentAttributes(sampler2D normalTexture,
                                         sampler2D albedoTexture,
                                         sampler2D depthTexture)
{
    FragmentAttributes attributes;
    {
        vec4 normalData = texture(normalTexture, v_textureCoords);
        attributes.normal = normalData.rgb;
        attributes.mediumType = blockTypeFromFloat(normalData.a);
    }
    attributes.albedo = texture(albedoTexture, v_textureCoords);
    attributes.screenSpaceDepth = texture(depthTexture, v_textureCoords).r;
    {
        vec4 clipSpacePosition = vec4(v_textureCoords * 2.0 - 1.0,
                                      attributes.screenSpaceDepth * 2.0 - 1.0,
                                      1.0);
        vec4 worldPosition = u_viewProjectionMatrixInverse * clipSpacePosition;
        attributes.worldPosition = worldPosition.xyz / worldPosition.w;
    }
    attributes.worldSpaceDepth = distance(attributes.worldPosition, u_cameraPosition);
    return attributes;
}

const vec3 LightDirection = normalize(vec3(0.5, 1.0, 0.75));

float getLightIntensity(vec3 normal)
{
    float diffuseTerm = max(dot(normal, LightDirection), 0.0);
    float ambientTerm = 0.2;
    return diffuseTerm + ambientTerm;
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
    FragmentAttributes opaqueAttributes = getFragmentAttributes(u_opaqueNormalTexture,
                                                                u_opaqueAlbedoTexture,
                                                                u_opaqueDepthTexture);
    FragmentAttributes translucentAttributes = getFragmentAttributes(u_translucentNormalTexture,
                                                                     u_translucentAlbedoTexture,
                                                                     u_translucentDepthTexture);

    float farDepth = opaqueAttributes.worldSpaceDepth;
    vec3 farColor;
    int farMediumType;
    if (opaqueAttributes.screenSpaceDepth >= 1.0) {
        farColor = SkyColor;
        farMediumType = BlockTypeAir;
    } else {
        farColor = opaqueAttributes.albedo.rgb * getLightIntensity(opaqueAttributes.normal);
        farMediumType = opaqueAttributes.mediumType;
    }

    float nearDepth;
    vec4 nearColor;
    int nearMediumType;
    if (translucentAttributes.screenSpaceDepth >= opaqueAttributes.screenSpaceDepth) {
        nearDepth = 0.0;
        nearColor = vec4(0.0, 0.0, 0.0, 0.0);
        nearMediumType = BlockTypeAir;
    } else {
        nearDepth = translucentAttributes.worldSpaceDepth;
        nearColor = translucentAttributes.albedo * getLightIntensity(translucentAttributes.normal);
        nearMediumType = translucentAttributes.mediumType;
    }

    vec3 finalColor = getAttenuatedColor(farColor, farDepth - nearDepth, farMediumType);
    finalColor = mix(finalColor, nearColor.rgb, nearColor.a);
    finalColor = getAttenuatedColor(finalColor, nearDepth, nearMediumType);
    f_color = vec4(finalColor, 1.0);
}
