#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_position;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in float _size[];
in float _rotation[];
out vec2 texcoord;

const vec3 shifts[4] = vec3[4](
    vec3(-1.0, -1.0, 0.0),
    vec3(-1.0, 1.0, 0.0),
    vec3(1.0, -1.0, 0.0),
    vec3(1.0, 1.0, 0.0)
);

void main() {
    vec3 center = gl_in[0].gl_Position.xyz;
    vec3 z = normalize(camera_position - center);
    vec3 y_ = vec3(0.0, 1.0, 0.0);
    vec3 x_ = normalize(cross(y_, z));

    vec3 x = x_ * cos(_rotation[0]) + y_ * sin(_rotation[0]);
    vec3 y = -x_ * sin(_rotation[0]) + y_ * cos(_rotation[0]);

    for(int i = 0; i < 4; i++) {
        vec3 cur_vertex = center;
        cur_vertex += shifts[i].x * _size[0] * x;
        cur_vertex += shifts[i].y * _size[0] * y;
        gl_Position = projection * view * model * vec4(cur_vertex, 1.0);
        texcoord = 0.5 * shifts[i].xy + vec2(0.5);
        EmitVertex();
    }
    EndPrimitive();
}