in float v_shadowViewSpaceDepth;

layout(location = 0) out vec2 f_depth;

void main()
{
    f_depth = vec2(v_shadowViewSpaceDepth, v_shadowViewSpaceDepth * v_shadowViewSpaceDepth);
}
