#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aPos;
    // Remove translation from the view to keep camera center at origin
    vec4 pos = projection * view * vec4(aPos, 1.0);
    // Force depth to the far plane, so skybox doesn't clip
    gl_Position = pos.xyww;
}