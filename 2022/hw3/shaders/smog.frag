#version 330 core

uniform sampler2D shadow_map;
uniform mat4 transform;
uniform vec3 camera_position;
uniform vec3 light_direction;
uniform vec3 center;
uniform float r;
uniform sampler3D cloud;

layout (location = 0) out vec4 out_color;

void sort(inout float x, inout float y) {
    if (x > y) {
        float t = x;
        x = y;
        y = t;
    }
}

vec2 intersect_sphere(vec3 origin, vec3 dir) {
    float a = (pow(dir[0], 2.0) + pow(dir[1], 2.0) + pow(dir[2], 2.0));
    float b = -2.0 * dot(dir, center - origin);
    float c = pow((center - origin)[0], 2.0) +
        pow((center - origin)[1], 2.0) +
        pow((center - origin)[2], 2.0) - pow(r, 2.0);
    float d = pow(b, 2.0) - 4.0 * a * c;
    float tmin = (-b + sqrt(d)) / (2.0 * a);
    float tmax = (-b - sqrt(d)) / (2.0 * a);
    sort(tmin, tmax);
    return vec2(tmin, tmax);
}

const float PI = acos(-1);

in vec3 position;

void main() {
    vec3 dir = normalize(position - camera_position);
    vec2 t = intersect_sphere(camera_position, dir);
    float tmin = t[0];
    float tmax = t[1];
    float absorption = 0.6f;
    float optical_depth = 0.0;
    float dt = (tmax - tmin) / 64.0;
    for(int i = 0; i < 64; i++) {
        float t = tmin + (float(i) + 0.5) * dt;
        vec3 p = camera_position + t * dir;

        vec4 shadow_pos = transform * vec4(p, 1.0);
        shadow_pos /= shadow_pos.w;
        shadow_pos = shadow_pos * 0.5 + vec4(0.5);

        bool in_shadow_texture = (shadow_pos.x > 0.0) && (shadow_pos.x < 1.0) &&
        (shadow_pos.y > 0.0) && (shadow_pos.y < 1.0) &&
        (shadow_pos.z > 0.0) && (shadow_pos.z < 1.0);

        int shadow_factor = 0;
        if (in_shadow_texture) {
            shadow_factor = int(texture(shadow_map, shadow_pos.xy).r < shadow_pos.z);
        }

        optical_depth += absorption * dt * (1 - shadow_factor);
    }
    float opacity = 1.0 - exp(-optical_depth);
    out_color = vec4(1.0, 1.0, 1.0, opacity);
}