#version 330 core

uniform vec3 color;

in vec3 position;
in vec3 normal;
in vec2 tex_coord;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(color, 1.0);
}