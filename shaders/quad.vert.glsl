const vec2 positions[4] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

const vec2 textureCoords[4] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

out vec2 v_textureCoords;

void main()
{
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    v_textureCoords = textureCoords[gl_VertexID];
}
