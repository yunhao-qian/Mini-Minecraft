#version 330

uniform bool u_isLiquid;
uniform sampler2D u_colorTexture;
uniform sampler2D u_normalTexture;

in vec2 v_textureCoords;
in vec3 v_normal;
in vec3 v_tangent;

out vec4 f_color;

const vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.75));

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
        tangentSpaceNormal.x = -tangentSpaceNormal.x;
        fragmentNormal = normalize(tbnMatrix * tangentSpaceNormal);
    }

    float diffuseTerm = max(dot(normalize(fragmentNormal), lightDirection), 0.0);
    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm;

    f_color = vec4(textureColor.rgb * lightIntensity, u_isLiquid ? 0.5 : 1.0);
}
