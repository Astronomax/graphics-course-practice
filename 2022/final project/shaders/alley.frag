#version 330 core

uniform sampler2D albedo;
uniform vec3 light_color;
uniform sampler2D normal_texture;
uniform sampler2D roughness_texture;
uniform vec3 camera_position;

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
    float roughness = texture(roughness_texture, texcoord).r;
    //float roughness = 1.0;
    float power = 1.0 / (roughness * roughness) - 1.0;
    vec3 reflected_direction = 2.0 * real_normal * dot(real_normal, direction) - direction;
    vec3 camera_direction = normalize(camera_position - position);
    float glossiness = 1.0f;
    return glossiness * pow(max(0.0, dot(reflected_direction, camera_direction)), power);
}

float phong(vec3 real_normal, vec3 direction) {
    return diffuse(real_normal, direction) + specular(real_normal, direction);
}

void main() {
    vec3 bitangent = cross(tangent, normal);
    mat3 tbn = mat3(tangent, bitangent, normal);
    vec3 real_normal = normalize(tbn * (texture(normal_texture, texcoord).xyz * 2.0 - vec3(1.0)));

    vec4 albedo_color;
    if (use_texture == 1)
    albedo_color = texture(albedo, texcoord);
    else
    albedo_color = color;

    float ambient = 0.8;
    vec3 light = ambient + light_color * phong(real_normal, light_direction);
    out_color = vec4(albedo_color.rgb * light, albedo_color.a);
}