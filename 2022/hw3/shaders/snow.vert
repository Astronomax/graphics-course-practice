#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in float size;
layout (location = 2) in float rotation;

out float _size;
out float _rotation;

void main() {
    gl_Position = vec4(in_position, 1.0);
    _size = size;
    _rotation = rotation;
}