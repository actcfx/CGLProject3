#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 u_model;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
};

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vClipSpace;

void main()
{
    vec4 worldPosition = u_model * vec4(aPos, 1.0);
    vWorldPos = worldPosition.xyz;
    vNormal = mat3(transpose(inverse(u_model))) * aNormal;
    vTexCoord = aTexCoord;

    vClipSpace = u_projection * u_view * worldPosition;
    gl_Position = vClipSpace;
}