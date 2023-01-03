#version 330 core

in vec3 position;
in vec3 normal;
in vec2 tex_coord;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(1.0, 0.0, 0.0, 1.0);
}