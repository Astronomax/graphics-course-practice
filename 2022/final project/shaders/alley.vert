#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_tangent;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec2 in_texcoord;

out vec3 position;
out vec3 normal;
out vec3 tangent;
out vec2 texcoord;

void main() {
    gl_Position = projection * view * model * vec4(in_position, 1.0);
    position = (model * vec4(in_position, 1.0)).xyz;
    tangent = mat3(model) * in_tangent;
    normal = normalize(mat3(model) * in_normal);
    texcoord = in_texcoord;
}