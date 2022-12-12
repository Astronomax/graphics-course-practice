#version 330 core

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

vec2 solve(float a, float b, float c) {
    float D = b * b - 4 * a * c;
    float x1 = (-b - sqrt(D)) / (2 * a);
    float x2 = (-b + sqrt(D)) / (2 * a);
    return vec2(x1, x2);
}

vec2 intersect_sphere(vec3 origin, vec3 direction) {
    vec3 origin_ = origin;
    origin_ -= center;
    float A = pow(direction.x, 2) + pow(direction.y, 2) + pow(direction.z, 2);
    float B = 2 * dot(origin_, direction);
    float C = pow(origin_.x, 2) + pow(origin_.y, 2) + pow(origin_.z, 2) - 1;
    return solve(A, B, C);
}

/*
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
*/
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
        float density = 0.2f;
        optical_depth += absorption * dt;
    }
    float opacity = 1.0 - exp(-optical_depth);
    out_color = vec4(1.0, 1.0, 1.0, opacity);
}