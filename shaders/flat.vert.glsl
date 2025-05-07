#version 330

uniform mat4 u_viewProjectionMatrix;
uniform mat4 u_modelMatrix;

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;

out vec4 v_color;

void main()
{
    v_color = a_color;
    gl_Position = u_viewProjectionMatrix * u_modelMatrix * a_position;
}
