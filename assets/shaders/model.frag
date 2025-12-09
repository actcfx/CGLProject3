#version 430 core

in vec2 vTexCoord;
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
};

uniform Material material;

void main() {
    FragColor = texture(material.texture_diffuse1, vTexCoord);
}
