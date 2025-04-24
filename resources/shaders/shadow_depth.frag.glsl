#version 410 core

in float v_shadowViewSpaceDepth;

layout(location = 0) out vec2 f_depth;

void main()
{
    f_depth.r = v_shadowViewSpaceDepth;
    f_depth.g = v_shadowViewSpaceDepth * v_shadowViewSpaceDepth;
}
