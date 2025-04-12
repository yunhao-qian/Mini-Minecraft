in float v_shadowViewSpaceDepth;

layout(location = 0) out float f_depth;

void main()
{
    f_depth = v_shadowViewSpaceDepth;
}
