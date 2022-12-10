#version 330 core
uniform mat4 model;
uniform mat4 transform;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_tangent;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec2 in_tex_coord;

out vec2 tex_coord;

void main() {
    gl_Position = transform * model * vec4(in_position, 1.0);
    tex_coord = vec2(in_tex_coord[0], 1.f - in_tex_coord[1]);
}