#version 330 core
uniform sampler2D alpha_texture;
uniform int have_alpha;

layout (location = 0) out vec4 out_color;

in vec2 tex_coord;

void main() {
    float z = gl_FragCoord.z;
    out_color = vec4(z, z * z + 0.25 * (pow(dFdx(z), 2.0) + pow(dFdy(z), 2.0)), 0.0, 0.0);
    if(have_alpha == 1 && texture(alpha_texture, tex_coord).r < 0.5) {
        discard;
    }
}