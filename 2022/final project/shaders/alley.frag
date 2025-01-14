#version 330 core

uniform sampler2D albedo;
uniform vec3 light_color;
uniform sampler2D normal_texture;
uniform sampler2D roughness_texture;
uniform vec3 camera_position;
uniform sampler2D shadow_map;
uniform mat4 transform;
uniform vec4 color;
uniform int use_texture;

uniform vec3 light_direction;

layout (location = 0) out vec4 out_color;

in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec2 texcoord;

float diffuse(vec3 real_normal, vec3 direction) {
    return max(0.0, dot(real_normal, direction));
}

float specular(vec3 real_normal, vec3 direction) {
    float roughness = texture(roughness_texture, texcoord).g;
    float power = 1.0 / pow(roughness, 2.0) - 1.0;
    vec3 reflected_direction = 2.0 * real_normal * dot(real_normal, direction) - direction;
    vec3 camera_direction = normalize(camera_position - position);
    float glossiness = 3.0;
    return glossiness * pow(max(0.0, dot(reflected_direction, camera_direction)), power);
}

float phong(vec3 real_normal, vec3 direction) {
    return diffuse(real_normal, direction) + specular(real_normal, direction);
}

void main() {
    vec3 bitangent = cross(tangent, normal);
    mat3 tbn = mat3(tangent, bitangent, normal);
    vec3 real_normal = normalize(tbn * (texture(normal_texture, texcoord).xyz * 2.0 - vec3(1.0)));


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
                float k = exp(-(pow(x, 2) + pow(y, 2)) / 18.0);
                a += k * texture(shadow_map, shadow_pos.xy + vec2(x, y) / textureSize(shadow_map, 0).xy).rg;
                b += k;
            }
        }
        vec2 data = a / b;

        float mu = data.r;
        float sigma = data.g - mu * mu;
        float z = shadow_pos.z - 0.001;
        shadow_factor = (z < mu) ? 1.0 : sigma / (sigma + (z - mu) * (z - mu));
        float delt = 0.125;
        if(shadow_factor < delt)
        shadow_factor = 0.0;
        else
        shadow_factor = (shadow_factor - delt) / (1.0 - delt);
    }


    vec4 albedo_color;
    if (use_texture == 1)
    albedo_color = texture(albedo, vec2(texcoord.x, texcoord.y));
    else
    albedo_color = color;

    float ambient = texture(roughness_texture, texcoord).r;
    //float koef = texture(roughness_texture, texcoord).b;
    vec3 light = vec3(ambient) + light_color * phong(real_normal, light_direction) * shadow_factor;
    out_color = vec4(albedo_color.rgb * light, albedo_color.a);
}