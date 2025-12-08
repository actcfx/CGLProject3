#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out float Height;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    FragPos = vec3(u_model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(u_model))) * aNormal;
    Height = aPos.y;

    gl_Position = u_projection * u_view * vec4(FragPos, 1.0);
}
