#version 330 core

uniform sampler2D albedo;
uniform sampler2D normal_texture;

uniform vec4 color;
uniform int use_texture;

uniform vec3 light_direction;

layout (location = 0) out vec4 out_color;

in vec3 tangent;
in vec3 normal;
in vec2 texcoord;

void main() {
    vec3 bitangent = cross(tangent, normal);
    mat3 tbn = mat3(tangent, bitangent, normal);
    vec3 real_normal = tbn * (texture(normal_texture, texcoord).xyz * 2.0 - vec3(1.0));

    vec4 albedo_color;
    if (use_texture == 1)
    albedo_color = texture(albedo, texcoord);
    else
    albedo_color = color;

    float ambient = 0.4;
    float diffuse = max(0.0, dot(normalize(real_normal), light_direction));

    out_color = vec4(albedo_color.rgb * (ambient + diffuse), albedo_color.a);
}