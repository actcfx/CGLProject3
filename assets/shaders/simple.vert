#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;
layout (location = 3) in vec3 color;

uniform mat4 u_model;
layout (std140, binding = 0) uniform common_matrices {
    mat4 u_projection;
    mat4 u_view;
};

out V_OUT {
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
   vec3 color;
} v_out;

void main() {
    vec4 worldPos4 = u_model * vec4(position, 1.0);
    vec3 worldNormal = normalize(mat3(transpose(inverse(u_model))) * normal);

    gl_Position = u_projection * u_view * worldPos4;

    v_out.position = worldPos4.xyz;
    v_out.normal = worldNormal;
    v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0 - texture_coordinate.y);
    v_out.color = color;
}