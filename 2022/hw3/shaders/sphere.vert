#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_tangent;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec2 in_tex_coord;

out vec3 position;
out vec3 normal;

void main() {
    position = (model * vec4(in_position, 1.0)).xyz;
    gl_Position = projection * view * model * vec4(position, 1.0);
    normal = normalize(mat3(model) * in_normal);
}