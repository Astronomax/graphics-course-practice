#version 330 core
uniform mat4 model;
uniform mat4 transform;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_tangent;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;

out vec3 position;

void main() {
    gl_Position = transform * model * vec4(in_position, 1.0);
    position = mat3(model) * in_position;
}