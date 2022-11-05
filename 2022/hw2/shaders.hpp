#ifndef HW2_SHADERS_HPP
#define HW2_SHADERS_HPP

const char shadow_vertex_shader_source[] = R"(#version 330 core
    uniform mat4 model;
    uniform mat4 transform;

    layout (location = 0) in vec3 in_position;

    void main() {
        gl_Position = transform * model * vec4(in_position, 1.0);
    }
)";

const char shadow_fragment_shader_source[] = R"(#version 330 core
    layout (location = 0) out vec4 out_color;

    void main() {
        float z = gl_FragCoord.z;
        out_color = vec4(z, z * z + 0.25 * (pow(dFdx(z), 2.0) + pow(dFdy(z), 2.0)), 0.0, 0.0);
    }
)";

const char vertex_shader_source[] = R"(#version 330 core
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec3 in_normal;
    layout (location = 2) in vec2 in_texcoord;

    out vec3 position;
    out vec3 normal;
    out vec2 texcoord;

    void main() {
        position = (model * vec4(in_position, 1.0)).xyz;
        gl_Position = projection * view * vec4(position, 1.0);
        normal = normalize(mat3(model) * in_normal);
        texcoord = vec2(in_texcoord[0], in_texcoord[1]);
    }
)";

const char fragment_shader_source[] = R"(#version 330 core
    uniform vec3 camera_position;
    uniform vec3 sun_direction;
    uniform vec3 sun_color;
    uniform sampler2D tex;

    in vec3 position;
    in vec3 normal;
    in vec2 texcoord;

    layout (location = 0) out vec4 out_color;

    vec3 diffuse(vec3 albedo, vec3 direction) {
        return albedo * max(0.0, dot(normal, direction));
    }
    vec3 specular(vec3 albedo, vec3 direction) {
        float power = 64.0;
        vec3 reflected_direction = 2.0 * normal * dot(normal, direction) - direction;
        vec3 view_direction = normalize(camera_position - position);
        return albedo * pow(max(0.0, dot(reflected_direction, view_direction)), power);
    }
    vec3 phong(vec3 albedo, vec3 direction) {
        return diffuse(albedo, direction) + specular(albedo, direction);
    }
    void main() {
        float ambient_light = 0.2;
        vec3 albedo = vec3(texture(tex, texcoord));
        vec3 color = albedo * ambient_light + sun_color * phong(albedo, sun_direction);
        out_color = vec4(color, 1.0);
    }
)";

#endif //HW2_SHADERS_HPP
