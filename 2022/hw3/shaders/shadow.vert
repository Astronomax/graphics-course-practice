#version 330 core
uniform mat4 model;
uniform mat4 transform;
uniform mat4x3 bones[64];
uniform int use_bones;

layout (location = 0) in vec3 in_position;
//layout (location = 1) in vec3 in_tangent;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord;
layout (location = 3) in ivec4 in_joints;
layout (location = 4) in vec4 in_weights;

out vec2 tex_coord;

void main() {
    if(use_bones == 1) {
        mat4x3 average = mat4x3(0.0);
        average += bones[in_joints.x] * in_weights.x;
        average += bones[in_joints.y] * in_weights.y;
        average += bones[in_joints.z] * in_weights.z;
        average += bones[in_joints.w] * in_weights.w;
        gl_Position = transform * model * mat4(average) * vec4(in_position, 1.0);
    }
    else {
        gl_Position = transform * model * vec4(in_position, 1.0);
    }
    tex_coord = vec2(in_tex_coord[0], 1.f - in_tex_coord[1]);
}