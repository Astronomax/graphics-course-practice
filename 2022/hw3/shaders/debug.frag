#version 330 core
uniform sampler2D shadow_map;

layout (location = 0) out vec4 out_color;

in vec2 texcoord;

void main() {
    out_color = vec4(texture(shadow_map, texcoord).rgb, 1.0);
}