#version 330

uniform vec3 u_cameraPosition;
uniform sampler2DArray u_colorTextureArray;
uniform sampler2DArray u_normalTextureArray;
uniform vec2 u_viewportSize;
uniform sampler2D u_solidBlocksColorTexture;
uniform sampler2D u_solidBlocksDepthTexture;

in vec3 v_position;
flat in int v_textureIndex;
in vec2 v_textureCoords;
in vec3 v_normal;
in vec3 v_tangent;
flat in int v_isWater;

out vec4 f_color;

const vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.75));

float linearizeDepth(float depth)
{
    const float zNear = 0.1;
    const float zFar = 1000.0;
    float z = depth * 2.0 - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z * (zFar - zNear));
}

void main()
{
    vec2 backgroundTextureCoords;
    float backgroundDepth;
    if (v_isWater != 0) {
        backgroundTextureCoords = gl_FragCoord.xy / u_viewportSize;
        backgroundDepth = texture(u_solidBlocksDepthTexture, backgroundTextureCoords).r;
        if (gl_FragCoord.z > backgroundDepth) {
            discard;
        }
    }

    vec4 textureColor = texture(u_colorTextureArray, vec3(v_textureCoords, float(v_textureIndex)));
    vec4 textureNormal = texture(u_normalTextureArray, vec3(v_textureCoords, float(v_textureIndex)));

    vec3 faceNormal = normalize(v_normal);
    vec3 fragmentNormal;
    if (textureNormal.a < 0.5) {
        fragmentNormal = faceNormal;
    } else {
        vec3 faceTangent = normalize(v_tangent);
        vec3 faceBitangent = normalize(cross(faceNormal, faceTangent));
        mat3 tbnMatrix = mat3(faceTangent, faceBitangent, faceNormal);
        vec3 tangentSpaceNormal = textureNormal.xyz * 2.0 - 1.0;
        tangentSpaceNormal.x = -tangentSpaceNormal.x;
        fragmentNormal = normalize(tbnMatrix * tangentSpaceNormal);
    }

    float diffuseTerm = max(dot(normalize(fragmentNormal), lightDirection), 0.0);
    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm;

    f_color = vec4(textureColor.rgb * lightIntensity, 1.0);

    if (v_isWater != 0) {
        float dotProduct = dot(faceNormal, normalize(u_cameraPosition - v_position));
        float waterLinearDepth = linearizeDepth(gl_FragCoord.z);
        float attenuationDistance;
        if (dotProduct > 0.0) {
            // Above water
            attenuationDistance = linearizeDepth(backgroundDepth) - waterLinearDepth;
        } else if (dotProduct < 0.0) {
            // Below water
            attenuationDistance = waterLinearDepth;
        } else {
            attenuationDistance = 0.0;
        }
        float attenuationFactor = clamp(exp(attenuationDistance * -0.1), 0.0, 1.0);

        vec3 backgroundColor = texture(u_solidBlocksColorTexture, backgroundTextureCoords).rgb;
        backgroundColor = mix(vec3(0.0, 0.4, 0.7), backgroundColor, attenuationFactor);

        f_color.rgb = mix(f_color.rgb, backgroundColor, 0.8);
    }
}
