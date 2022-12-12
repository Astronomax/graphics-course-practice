#version 330 core
uniform vec3 ambient;
uniform vec3 light_color;
uniform vec3 light_direction;
uniform vec3 camera_position;

uniform sampler2D environment_map_texture;

in vec3 position;
in vec3 normal;

layout (location = 0) out vec4 out_color;

const float PI = acos(-1);

void main() {
    vec3 direction = normalize(camera_position - position);
    vec3 reflected_direction = 2.0 * normal * dot(normal, direction) - direction;
    vec2 env_map_coord = vec2(atan(reflected_direction.z, reflected_direction.x) / PI * 0.5 + 0.5,
    -atan(reflected_direction.y, length(reflected_direction.xz)) / PI + 0.5);
    vec3 env_color = texture(environment_map_texture, env_map_coord).rgb;
    vec3 lightness = ambient + light_color * max(0.0, dot(normal, light_direction));
    float n = 3.5;
    float r0 = pow((1.0 - n) / (1.0 + n), 2.0);
    float r = r0 + (1.0 - r0) * pow(1.0 - dot(normal, direction), 5.0);
    out_color = vec4(lightness *  env_color, r);
}