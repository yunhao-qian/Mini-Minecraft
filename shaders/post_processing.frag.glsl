#version 330 core

in vec2 v_textureCoords;

uniform sampler2D u_solidBlocksColorTexture;
uniform sampler2D u_solidBlocksDepthTexture;
uniform sampler2D u_liquidBlocksColorTexture;
uniform sampler2D u_liquidBlocksDepthTexture;

out vec4 f_color;

float linearizeDepth(float depth)
{
    const float zNear = 0.1;
    const float zFar = 1000.0;
    float z = depth * 2.0 - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z * (zFar - zNear));
}

void main()
{
    vec4 solidColor = texture(u_solidBlocksColorTexture, v_textureCoords);
    float rawSolidDepth = texture(u_solidBlocksDepthTexture, v_textureCoords).r;
    vec4 liquidColor = texture(u_liquidBlocksColorTexture, v_textureCoords);
    float rawLiquidDepth = texture(u_liquidBlocksDepthTexture, v_textureCoords).r;

    f_color = vec4(0.37, 0.74, 1.0, 1.0);
    float attenuationDistance = 0.0;
    if (rawLiquidDepth < rawSolidDepth) {
        if (rawLiquidDepth <= 1.0) {
            f_color = liquidColor;
            attenuationDistance = linearizeDepth(rawLiquidDepth);
        }
    } else if (rawSolidDepth < rawLiquidDepth) {
        if (rawSolidDepth <= 1.0) {
            f_color = solidColor;
            attenuationDistance = linearizeDepth(rawSolidDepth);
        }
    }

    const float airMedia = 0.2;
    const float waterMedia = 0.4;
    const float lavaMedia = 0.6;

    vec3 attenuationFactor;
    vec3 attenuationColor;
    if (abs(f_color.a - airMedia) < 0.1) {
        attenuationFactor = vec3(0.002, 0.002, 0.002);
        attenuationColor = vec3(0.37, 0.74, 1.0);
    } else if (abs(f_color.a - waterMedia) < 0.1) {
        attenuationFactor = vec3(0.12, 0.08, 0.04);
        attenuationColor = vec3(0.2, 0.4, 0.6);
    } else if (abs(f_color.a - lavaMedia) < 0.1) {
        attenuationFactor = vec3(0.08, 0.16, 0.24);
        attenuationColor = vec3(1.0, 0.4, 0.0);
    } else {
        attenuationFactor = vec3(0.0, 0.0, 0.0);
        attenuationColor = vec3(0.37, 0.74, 1.0);
    }
    vec3 attenuation = clamp(exp(-attenuationDistance * attenuationFactor), 0.0, 1.0);
    f_color.rgb = mix(attenuationColor, f_color.rgb, attenuation);

    f_color.rgb = mix(f_color.rgb, vec3(0.37, 0.74, 1.0), smoothstep(448, 512, attenuationDistance));
    f_color.a = 1.0;
}
