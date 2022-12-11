#version 330 core

uniform mat4 transform;
uniform sampler2D albedo;
uniform sampler2D shadow_map;
uniform vec4 color;
uniform int use_texture;
uniform vec3 light_direction;
uniform vec3 light_color;

layout (location = 0) out vec4 out_color;

in vec3 position;
in vec3 normal;
in vec2 texcoord;

float diffuse(vec3 direction) {
    return max(0.0, dot(normal, direction));
}

void main() {
    vec4 albedo_color;

    if (use_texture == 1)
    albedo_color = texture(albedo, texcoord);
    else
    albedo_color = color;

    vec4 shadow_pos = transform * vec4(position, 1.0);
    shadow_pos /= shadow_pos.w;
    shadow_pos = shadow_pos * 0.5 + vec4(0.5);

    bool in_shadow_texture = (shadow_pos.x > 0.0) && (shadow_pos.x < 1.0) &&
    (shadow_pos.y > 0.0) && (shadow_pos.y < 1.0) &&
    (shadow_pos.z > 0.0) && (shadow_pos.z < 1.0);

    float shadow_factor = 1.0;
    if (in_shadow_texture) {
        vec2 a = vec2(0.0, 0.0);
        float b = 0.0;
        for (int x = -3; x <= 3; ++x) {
            for (int y = -3; y <= 3; ++y) {
                float k = exp(-(pow(x, 2) + pow(y, 2)) / 8.0);
                a += k * texture(shadow_map, shadow_pos.xy + vec2(x, y) / textureSize(shadow_map, 0).xy).rg;
                b += k;
            }
        }
        vec2 data = a / b;

        float mu = data.r;
        float sigma = data.g - mu * mu;
        float z = shadow_pos.z - 0.005;
        shadow_factor = (z < mu) ? 1.0 : sigma / (sigma + (z - mu) * (z - mu));
        float delt = 0.125;
        if(shadow_factor < delt)
        shadow_factor = 0.0;
        else
        shadow_factor = (shadow_factor - delt) / (1.0 - delt);
    }

    float ambient = 0.4;
    vec3 light = ambient + light_color * diffuse(light_direction) * shadow_factor;
    out_color = vec4(albedo_color.rgb * light, albedo_color.a);
}