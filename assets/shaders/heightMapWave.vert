#version 430 core
layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

out vec3 vs_worldpos;
out vec3 vs_normal;
out vec2 vs_texcoord;

uniform mat4 u_model;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
};

uniform sampler2D u_texture;
uniform float u_time;
uniform vec2 u_scroll;
uniform float u_heightScale;

void main(void){
    vec4 pos = position;

    // Calculate texture coordinates with scrolling
    // Use two scrolling directions for more chaotic look
    vec2 uv1 = texcoord + u_time * u_scroll;
    vec2 uv2 = texcoord - u_time * u_scroll * 0.5;

    // Sample height from texture (combine two samples)
    float h1 = texture(u_texture, uv1).r;
    float h2 = texture(u_texture, uv2).r;
    float height = (h1 + h2) * 0.5;

    // Displace vertex
    pos.y += height * u_heightScale;

    // Calculate normal using finite difference
    float offset = 0.01;
    float h_right = (texture(u_texture, uv1 + vec2(offset, 0.0)).r + texture(u_texture, uv2 + vec2(offset, 0.0)).r) * 0.5;
    float h_up = (texture(u_texture, uv1 + vec2(0.0, offset)).r + texture(u_texture, uv2 + vec2(0.0, offset)).r) * 0.5;

    float dIdx = (h_right - height) / offset;
    float dIdz = (h_up - height) / offset;

    // Normal = normalize(-dy/dx, 1, -dy/dz)
    vec3 newNormal = normalize(vec3(-dIdx * u_heightScale, 1.0, -dIdz * u_heightScale));

    vec4 worldPos = u_model * pos;
    gl_Position = u_projection * u_view * worldPos;
    vs_worldpos = worldPos.xyz;
    vs_normal = normalize(mat3(u_model) * newNormal);
    vs_texcoord = texcoord; // Pass original texcoord for shading if needed
}