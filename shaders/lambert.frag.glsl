#version 330

in vec3 v_position;
in vec3 v_normal;
in vec3 v_color;

out vec4 f_color;

const vec3 lightDirection = normalize(vec3(0.5, 1.0, 0.75));

void main() {
    float diffuseTerm = dot(normalize(v_normal), lightDirection);
    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm;
    f_color = vec4(v_color * lightIntensity, 1.0);
}
