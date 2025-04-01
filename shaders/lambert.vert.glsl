#version 330

uniform mat4 u_viewProjectionMatrix;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_color;

out vec3 v_position;
out vec3 v_normal;
out vec3 v_color;

void main()
{
    v_position = a_position;
    v_normal = a_normal;
    v_color = a_color;
    gl_Position = u_viewProjectionMatrix * vec4(a_position, 1.0);
}
