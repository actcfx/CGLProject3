#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;

out V_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texture_coordinate;
} v_out;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    v_out.position = vec3(u_model * vec4(position, 1.0));
    v_out.normal = mat3(transpose(inverse(u_model))) * normal;
    v_out.texture_coordinate = texture_coordinate;

    gl_Position = u_projection * u_view * vec4(v_out.position, 1.0);
}
