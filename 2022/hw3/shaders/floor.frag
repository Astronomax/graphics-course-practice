#version 330 core

uniform vec3 light_direction;
uniform vec3 camera_position;

in vec3 position;
in vec3 tangent;
in vec3 normal;

layout (location = 0) out vec4 out_color;

const float PI = acos(-1);

void main() {
    vec3 direction = normalize(camera_position - position);
    float ambient_light = 0.2;
    float lightness = ambient_light + max(0.0, dot(normal, light_direction));
    out_color = vec4(vec3(lightness), 1.0);
}