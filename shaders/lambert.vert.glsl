#version 330

uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_textureCoords;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;

out vec2 v_textureCoords;
out vec3 v_normal;
out vec3 v_tangent;

void main()
{
    v_textureCoords = a_textureCoords;
    v_normal = a_normal;
    v_tangent = a_tangent;
    gl_Position = u_projectionMatrix * u_viewMatrix * vec4(a_position, 1.0);
}
