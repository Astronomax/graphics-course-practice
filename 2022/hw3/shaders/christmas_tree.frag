#version 330 core
uniform vec3 ambient;

uniform vec3 light_direction;
uniform vec3 light_color;
uniform mat4 transform;
uniform vec3 ambient_color;
uniform sampler2D shadow_map;
uniform sampler2D ambient_texture;
uniform sampler2D alpha_texture;
uniform float glossiness;
uniform float power;
uniform vec3 camera_position;
uniform int have_alpha;
uniform int have_ambient_texture;

in vec3 position;
in vec3 normal;
in vec2 tex_coord;

layout (location = 0) out vec4 out_color;

float diffuse(vec3 direction) {
    return max(0.0, dot(normal, direction));
}

float specular(vec3 direction) {
    vec3 reflected_direction = 2.0 * normal * dot(normal, direction) - direction;
    vec3 camera_direction = normalize(camera_position - position);
    return glossiness * pow(max(0.0, dot(reflected_direction, camera_direction)), power);
}

float phong(vec3 direction) {
    return diffuse(direction) + specular(direction);
}

void main() {
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


    vec3 albedo = ambient_color;
    if(have_ambient_texture == 1) {
        albedo = vec3(texture(ambient_texture, tex_coord));
    }
    vec3 light = ambient + light_color /** phong(light_direction)*/ * shadow_factor;
    vec3 color = albedo * light;
    out_color = vec4(color, 1.0);

    if(have_alpha == 1 && texture(alpha_texture, tex_coord).r < 0.5) {
        discard;
    }
}