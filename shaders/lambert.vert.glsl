#version 330

uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform float u_time;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_textureCoords;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in int a_isWater;
layout(location = 5) in int a_isLava;

out vec2 v_textureCoords;
out vec3 v_normal;
out vec3 v_tangent;
out float v_opacity;

vec2 randomOffset(float frequency)
{
    float discreteTime = floor(u_time * frequency) / frequency;
    vec2 offset = fract(sin(vec2(discreteTime * 127.1, discreteTime * 269.5)) * 43758.5453);
    vec2 discreteOffset = floor(offset * 16.0) / 16.0;
    return discreteOffset / 16.0;
}

void main()
{
    v_textureCoords = a_textureCoords;
    if (a_isWater != 0) {
        v_textureCoords += randomOffset(8.0);
        v_opacity = 0.5;
    } else {
        v_opacity = 1.0;
        if (a_isLava != 0) {
            v_textureCoords += randomOffset(4.0);
        }
    }
    v_normal = a_normal;
    v_tangent = a_tangent;
    gl_Position = u_projectionMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
