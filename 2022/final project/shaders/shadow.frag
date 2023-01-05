#version 330 core

layout (location = 0) out vec4 out_color;

in vec3 position;

void main() {
    float z = gl_FragCoord.z;
    out_color = vec4(z, z * z + 0.25 * (pow(dFdx(z), 2.0) + pow(dFdy(z), 2.0)), 0.0, 0.0);
    //if(position.y > 0.5)
    //discard;
}