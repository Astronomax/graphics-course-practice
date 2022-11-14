#ifndef HW2_SHADERS_HPP
#define HW2_SHADERS_HPP

const char shadow_vertex_shader_source[] = R"(#version 330 core
    uniform mat4 model;
    uniform mat4 transform;

    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec3 in_normal;
    layout (location = 2) in vec2 in_tex_coord;

    out vec2 tex_coord;

    void main() {
        gl_Position = transform * model * vec4(in_position, 1.0);
        tex_coord = vec2(in_tex_coord[0], 1.f - in_tex_coord[1]);
    }
)";

const char shadow_fragment_shader_source[] = R"(#version 330 core
    uniform sampler2D alpha_texture;
    uniform int have_alpha;

    layout (location = 0) out vec4 out_color;

    in vec2 tex_coord;

    void main() {
        float z = gl_FragCoord.z;
        out_color = vec4(z, z * z + 0.25 * (pow(dFdx(z), 2.0) + pow(dFdy(z), 2.0)), 0.0, 0.0);
        if(have_alpha == 1 && texture(alpha_texture, tex_coord).r < 0.5) {
            discard;
        }
    }
)";

const char vertex_shader_source[] = R"(#version 330 core
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec3 in_normal;
    layout (location = 2) in vec2 in_tex_coord;

    out vec3 position;
    out vec3 normal;
    out vec2 tex_coord;

    void main() {
        gl_Position = projection * view * model * vec4(in_position, 1.0);
        position = (model * vec4(in_position, 1.0)).xyz;
        normal = normalize((model * vec4(in_normal, 0.0)).xyz);
        tex_coord = vec2(in_tex_coord[0], 1.f - in_tex_coord[1]);
    }
)";

const char fragment_shader_source[] = R"(#version 330 core
    uniform vec3 ambient;

    uniform vec3 light_direction;
    uniform vec3 light_color;
    uniform mat4 transform;
    uniform sampler2D shadow_map;
    uniform sampler2D ambient_texture;
    uniform sampler2D alpha_texture;
    uniform float glossiness;
    uniform float power;
    uniform vec3 camera_position;
    uniform int have_alpha;

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

        vec3 albedo = vec3(texture(ambient_texture, tex_coord));
        //vec3 point_light_direction = normalize(position - point_light_position);
        vec3 light = ambient + light_color * phong(light_direction) * shadow_factor;
        vec3 color = albedo * light;
        out_color = vec4(color, 1.0);

        if(have_alpha == 1 && texture(alpha_texture, tex_coord).r < 0.5) {
            discard;
        }
    }
)";

#endif //HW2_SHADERS_HPP
