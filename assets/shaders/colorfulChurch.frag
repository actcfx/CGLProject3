#version 430 core
out vec4 f_color;

in V_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texture_coordinate;
    vec3 color;
    vec4 lightSpacePos;
} f_in;

uniform vec3 u_color;

uniform sampler2D u_texture;
uniform vec3 u_cameraPos;
uniform vec2 u_smokeParams;
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

void main()
{
    vec3 texColor = vec3(texture(u_texture, f_in.texture_coordinate));
    vec3 baseColor = mix(texColor, f_in.color, 0.5);

    float shadow = 0.0;
    if (u_enableShadow) {
        shadow = computeShadow(f_in.normal, f_in.lightSpacePos);
    }
    baseColor *= (1.0 - 0.7 * shadow);

    float smoke = 0.0;
    if (u_smokeParams.y > u_smokeParams.x && u_smokeParams.x >= 0.0) {
        float dist = length(u_cameraPos - f_in.position);
        smoke = clamp((dist - u_smokeParams.x) /
                          max(u_smokeParams.y - u_smokeParams.x, 0.0001),
                      0.0, 1.0);
    }

    if (!smokeEnabled) smoke = 0.0;
    vec3 finalColor = mix(baseColor, vec3(1.0), smoke);
    f_color = vec4(finalColor, 1.0f);
}