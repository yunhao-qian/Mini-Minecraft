#version 330

uniform sampler2DArray u_colorTextureArray;
uniform sampler2DArray u_normalTextureArray;

flat in int v_textureIndex;
in vec2 v_textureCoords;
in vec3 v_normal;
in vec3 v_tangent;
in float v_opacity;

out vec4 f_color;

const vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.75));

void main()
{
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

    f_color = vec4(textureColor.rgb * lightIntensity, v_opacity);
}
