#version 430 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
};

uniform Material material;
uniform vec3 uCameraPos;
uniform vec2 uSmokeParams;
uniform int uShadowPass;

void main() {
    if (uShadowPass == 1) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.5);
        return;
    }

    vec4 baseColor = texture(material.texture_diffuse1, vTexCoord);

    float smoke = 0.0;
    if (uSmokeParams.y > uSmokeParams.x && uSmokeParams.x >= 0.0) {
        float dist = length(uCameraPos - vWorldPos);
        smoke = clamp((dist - uSmokeParams.x) /
                          max(uSmokeParams.y - uSmokeParams.x, 0.0001),
                      0.0, 1.0);
    }

    vec3 finalColor = mix(baseColor.rgb, vec3(1.0), smoke);
    FragColor = vec4(finalColor, baseColor.a);
}
