#version 330

uniform sampler2D u_colorTexture;
uniform sampler2D u_normalTexture;

in vec3 v_worldPosition;
in vec3 v_viewPosition;
in vec2 v_textureCoords;
in vec3 v_normal;
in vec3 v_tangent;

out vec4 f_color;

const vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.75));
const vec3 backgroundColor = vec3(0.37, 0.74, 1.0);

void main()
{
    vec4 textureColor = texture(u_colorTexture, v_textureCoords);
    vec4 textureNormal = texture(u_normalTexture, v_textureCoords);

    vec3 faceNormal = normalize(v_normal);
    vec3 fragmentNormal;
    if (textureNormal.a < 0.5) {
        fragmentNormal = faceNormal;
    } else {
        vec3 faceTangent = normalize(v_tangent);
        vec3 faceBitangent = normalize(cross(faceNormal, faceTangent));
        mat3 tbnMatrix = mat3(faceTangent, faceBitangent, faceNormal);
        vec3 tangentSpaceNormal = textureNormal.xyz * 2.0 - 1.0;
        fragmentNormal = normalize(tbnMatrix * tangentSpaceNormal);
    }

    float diffuseTerm = max(dot(normalize(fragmentNormal), lightDirection), 0.0);
    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm;

    float distanceToCamera = length(v_viewPosition);
    float opacity = exp(distanceToCamera * -0.001);
    opacity -= smoothstep(192.0, 256.0, distanceToCamera) * exp(256 * -0.001);
    opacity = clamp(opacity, 0.0, 1.0);

    f_color = vec4(mix(backgroundColor, textureColor.rgb * lightIntensity, opacity), 1.0);
}
