#version 330 core
uniform vec3 ambient;
uniform vec3 camera_position;
uniform sampler2D environment_map_texture;

uniform mat4 rotation;

layout (location = 0) out vec4 out_color;

in vec3 position;

const float PI = acos(-1);

void main() {
    vec3 direction = position - camera_position;
    direction = mat3(rotation) * direction;
    vec2 env_map_coord = vec2(atan(direction.z, direction.x) / PI * 0.5 + 0.5,
    -atan(direction.y, length(direction.xz)) / PI + 0.5);
    vec3 env_color = texture(environment_map_texture, env_map_coord).rgb;
    out_color = vec4(ambient * env_color, 1.0);
}