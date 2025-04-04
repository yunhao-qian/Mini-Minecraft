#version 330

uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_textureCoords;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;

out vec3 v_worldPosition;
out vec3 v_viewPosition;
out vec2 v_textureCoords;
out vec3 v_normal;
out vec3 v_tangent;

void main()
{
    v_worldPosition = a_position;
    v_viewPosition = (u_viewMatrix * vec4(v_worldPosition, 1.0)).xyz;
    v_textureCoords = a_textureCoords;
    v_normal = a_normal;
    v_tangent = a_tangent;
    gl_Position = u_projectionMatrix * vec4(v_viewPosition, 1.0);
}
