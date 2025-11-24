#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

uniform mat4 u_model;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
};

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 ClipSpace;

void main()
{
    vec4 worldPosition = u_model * vec4(aPos, 1.0);
    WorldPos = worldPosition.xyz;
    Normal = mat3(transpose(inverse(u_model))) * aNormal;
    TexCoord = aTexCoord;

    ClipSpace = u_projection * u_view * worldPosition;
    gl_Position = ClipSpace;
}