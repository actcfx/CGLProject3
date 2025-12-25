#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;
layout (location = 3) in vec3 barycentric;

uniform mat4 u_model;

layout (std140, binding = 0) uniform commom_matrices {
    mat4 u_projection;
    mat4 u_view;
};

out V_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 bary;
    vec2 uv;
} v_out;

void main() {
    vec4 world = u_model * vec4(position, 1.0);
    gl_Position = u_projection * u_view * world;

    v_out.worldPos = world.xyz;
    v_out.normal = mat3(transpose(inverse(u_model))) * normal;
    v_out.bary = barycentric;
    v_out.uv = texture_coordinate;
}
