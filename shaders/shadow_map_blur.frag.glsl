uniform int u_isVerticalBlur;
uniform float u_blurRadius;
uniform sampler2DArray u_shadowDepthTexture;
uniform int u_shadowMapCascadeIndex;

in vec2 v_textureCoords;

layout(location = 0) out vec2 f_blurredDepth;

void main()
{
    const int HalfNumSamples = 8;
    float offsetStep = u_blurRadius / float(HalfNumSamples);
    float multiplier = 1.0 / float(HalfNumSamples * 2 + 1);
    f_blurredDepth = vec2(0.0);
    for (int i = -HalfNumSamples; i <= HalfNumSamples; ++i) {
        float offset = float(i) * offsetStep;
        vec2 sampleCoords = v_textureCoords;
        if (u_isVerticalBlur == 0) {
            sampleCoords.x += offset;
        } else {
            sampleCoords.y += offset;
        }
        vec2 depth
            = texture(u_shadowDepthTexture, vec3(sampleCoords, float(u_shadowMapCascadeIndex))).xy;
        f_blurredDepth += depth * multiplier;
    }
}
