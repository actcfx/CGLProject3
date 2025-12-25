#version 430 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vWorldPos;
in vec4 vLightSpacePos;
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
};

uniform Material material;
uniform vec3 uCameraPos;
uniform vec2 uSmokeParams;
uniform int uShadowPass;
uniform bool smokeEnabled;

uniform sampler2D u_shadowMap;
uniform vec3 u_lightDir;
uniform bool u_enableShadow;

float computeShadow(vec3 normal, vec4 lightSpacePos) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
        return 0.0;

    vec3 N = normalize(normal);
    vec3 L = normalize(-u_lightDir);
    float bias = max(0.0009, 0.0015 * (1.0 - dot(N, L)));

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_shadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_shadowMap,
                                     projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main() {
    if (uShadowPass == 1) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.5);
        return;
    }

    vec4 baseColor = texture(material.texture_diffuse1, vTexCoord);

    // Clip texels that should be masked out (matches glTF alphaCutoff=0.05).
    if (baseColor.a < 0.05)
        discard;

    float smoke = 0.0;
    if (uSmokeParams.y > uSmokeParams.x && uSmokeParams.x >= 0.0) {
        float dist = length(uCameraPos - vWorldPos);
        smoke = clamp((dist - uSmokeParams.x) /
                          max(uSmokeParams.y - uSmokeParams.x, 0.0001),
                      0.0, 1.0);
    }

    if (!smokeEnabled) smoke = 0.0;
    float shadow = 0.0;
    if (u_enableShadow) {
        shadow = computeShadow(vNormal, vLightSpacePos);
    }

    vec3 litColor = baseColor.rgb * (1.0 - 0.7 * shadow);
    vec3 finalColor = mix(litColor, vec3(1.0), smoke);
    FragColor = vec4(finalColor, baseColor.a);
}
