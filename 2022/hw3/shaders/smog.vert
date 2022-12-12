#version 330 core

uniform mat4 view;
uniform mat4 projection;

uniform vec3 center;
uniform float r;

layout (location = 0) in vec3 in_position;
//layout (location = 1) in vec3 in_tangent;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;

out vec3 position;

void main() {
    position = vec4(in_position, 1.0).xyz;
    gl_Position = projection * view * vec4(position, 1.0);
}