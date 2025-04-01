#version 330

in vec3 v_position;
in vec3 v_normal;
in vec3 v_color;

out vec4 f_color;

const vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.75));

void main()
{
    float diffuseTerm = max(dot(normalize(v_normal), lightDirection), 0.0);

    bool xBound = fract(v_position.x) < 0.0125 || fract(v_position.x) > 0.9875;
    bool yBound = fract(v_position.y) < 0.0125 || fract(v_position.y) > 0.9875;
    bool zBound = fract(v_position.z) < 0.0125 || fract(v_position.z) > 0.9875;
    if ((xBound && yBound) || (xBound && zBound) || (yBound && zBound)) {
        diffuseTerm = 0.0;
    }

    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm;
    f_color = vec4(v_color * lightIntensity, 1.0);
}
