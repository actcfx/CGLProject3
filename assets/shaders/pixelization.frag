#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform float pixelSize;

uniform float screenWidth;
uniform float screenHeight;

void main()
{
    vec2 resolution = vec2(screenWidth, screenHeight);

    vec2 grid = resolution / pixelSize;

    vec2 uv = floor(TexCoords * grid) / grid;

    uv += (1.0 / grid) * 0.5;

    FragColor = texture(screenTexture, uv);
}