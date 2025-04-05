#version 330 core

uniform vec3 u_cameraPosition;
uniform sampler2DArray u_colorTextureArray;
uniform sampler2DArray u_normalTextureArray;
uniform vec2 u_viewportSize;
uniform sampler2D u_solidBlocksColorTexture;
uniform sampler2D u_solidBlocksDepthTexture;

in vec3 v_position;
flat in int v_textureIndex;
in vec2 v_textureCoords;
in vec3 v_normal;
in vec3 v_tangent;
flat in int v_isWater;
flat in int v_isLava;
flat in int v_isAdjacentToWater;
flat in int v_isAdjacentToLava;

out vec4 f_color;

const vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.75));

float linearizeDepth(float depth)
{
    const float zNear = 0.1;
    const float zFar = 1000.0;
    float z = depth * 2.0 - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z * (zFar - zNear));
}

void main()
{
    vec2 backgroundTextureCoords;
    float backgroundDepth;
    if (v_isWater != 0 || v_isLava != 0) {
        backgroundTextureCoords = gl_FragCoord.xy / u_viewportSize;
        backgroundDepth = texture(u_solidBlocksDepthTexture, backgroundTextureCoords).r;
        if (gl_FragCoord.z > backgroundDepth) {
            discard;
        }
    }

    vec4 textureColor = texture(u_colorTextureArray, vec3(v_textureCoords, float(v_textureIndex)));
    vec4 textureNormal = texture(u_normalTextureArray, vec3(v_textureCoords, float(v_textureIndex)));

    vec3 faceNormal = normalize(v_normal);
    vec3 fragmentNormal;
    if (textureNormal.a < 0.5) {
        fragmentNormal = faceNormal;
    } else {
        vec3 faceTangent = normalize(v_tangent);
        vec3 faceBitangent = normalize(cross(faceNormal, faceTangent));
        mat3 tbnMatrix = mat3(faceTangent, faceBitangent, faceNormal);
        vec3 tangentSpaceNormal = textureNormal.xyz * 2.0 - 1.0;
        tangentSpaceNormal.x = -tangentSpaceNormal.x;
        fragmentNormal = normalize(tbnMatrix * tangentSpaceNormal);
    }

    float diffuseTerm = max(dot(normalize(fragmentNormal), lightDirection), 0.0);
    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm;

    f_color = vec4(textureColor.rgb * lightIntensity, 1.0);

    // Use the spare alpha channel to encode media types.
    const float airMedia = 0.2;
    const float waterMedia = 0.4;
    const float lavaMedia = 0.6;

    bool isFrontFace = dot(faceNormal, normalize(u_cameraPosition - v_position)) > 0.0;

    if (v_isWater != 0) {
        vec3 backgroundColor = texture(u_solidBlocksColorTexture, backgroundTextureCoords).rgb;
        float attenuationDistance = linearizeDepth(backgroundDepth)
                                    - linearizeDepth(gl_FragCoord.z);
        vec3 attenuationFactor;
        vec3 attenuationColor;
        if (isFrontFace) {
            attenuationFactor = vec3(0.12, 0.08, 0.04);
            attenuationColor = vec3(0.2, 0.4, 0.6);
        } else {
            attenuationFactor = vec3(0.002, 0.002, 0.002);
            vec3(0.37, 0.74, 1.0);
        }
        vec3 attenuation = clamp(exp(-attenuationDistance * attenuationFactor), 0.0, 1.0);
        backgroundColor = mix(attenuationColor,
                              backgroundColor,
                              attenuation); // Mix with the background color based on attenuation.
        f_color.rgb = mix(f_color.rgb, backgroundColor, 0.8);

        f_color.a = isFrontFace ? airMedia : waterMedia;
    } else if (v_isLava != 0) {
        f_color.a = isFrontFace ? airMedia : lavaMedia;
    } else if (v_isAdjacentToWater != 0) {
        f_color.a = isFrontFace ? waterMedia : airMedia;
    } else if (v_isAdjacentToLava != 0) {
        f_color.a = isFrontFace ? lavaMedia : airMedia;
    } else {
        f_color.a = airMedia;
    }
}
