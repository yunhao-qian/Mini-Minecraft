#version 330

uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_color;

out vec3 v_worldPosition;
out vec3 v_viewPosition;
out vec3 v_normal;
out vec3 v_color;

void main()
{
    v_worldPosition = a_position;
    v_viewPosition = (u_viewMatrix * vec4(v_worldPosition, 1.0)).xyz;
    v_normal = a_normal;
    v_color = a_color;
    gl_Position = u_projectionMatrix * vec4(v_viewPosition, 1.0);
}
