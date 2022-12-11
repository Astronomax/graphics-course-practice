#version 330 core
uniform sampler2D _texture;
//uniform sampler1D gradient;

layout (location = 0) out vec4 out_color;

in vec2 texcoord;

void main() {
    float p = texture(_texture, texcoord).r;
    out_color = vec4(1.f, 1.f, 1.f, p);
}