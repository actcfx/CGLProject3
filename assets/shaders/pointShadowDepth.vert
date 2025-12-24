#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 u_model;
uniform mat4 u_lightMatrix;

out vec3 vWorldPos;

void main() {
    vec4 world = u_model * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    gl_Position = u_lightMatrix * world;
}
