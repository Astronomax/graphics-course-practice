#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4x3 bones[64];

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;
layout (location = 3) in ivec4 in_joints;
layout (location = 4) in vec4 in_weights;

out vec3 position;
out vec3 normal;
out vec2 texcoord;

void main() {
    mat4x3 average = mat4x3(0.0);
    average += bones[in_joints.x] * in_weights.x;
    average += bones[in_joints.y] * in_weights.y;
    average += bones[in_joints.z] * in_weights.z;
    average += bones[in_joints.w] * in_weights.w;
    position = (model * vec4(in_position, 1.0)).xyz;
    gl_Position = projection * view * model * mat4(average) * vec4(in_position, 1.0);
    normal = mat3(model) * mat3(average) * in_normal;
    texcoord = in_texcoord;
}