#version 430 core
layout (location = 0) out vec4 out_color;
in vec3 vs_worldpos;
in vec3 vs_normal;
in vec4 vLightSpacePos;

uniform vec4 color_ambient = vec4(0.1, 0.2, 0.5, 1.0);
uniform vec4 color_diffuse = vec4(0.0, 0.2, 0.7, 1.0); // Adjusted to blue
uniform vec4 color_specular = vec4(1.0, 1.0, 1.0, 1.0);
uniform vec4 color = vec4(0.0, 0.2, 0.7, 1.0); // Adjusted to blue
uniform float shininess = 50.0f;
uniform vec3 light_position = vec3(50.0f, 32.0f, 560.0f);
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

void main(void){
    vec3 light_direction = normalize(light_position - vs_worldpos);
    vec3 view_direction = normalize(u_cameraPos - vs_worldpos);
    vec3 half_vector = normalize(light_direction + view_direction);

    float diffuse = max(0.0, dot(vs_normal, light_direction));
    float specular = pow(max(0.0, dot(vs_normal, half_vector)), shininess);

    out_color = min(color * color_ambient + diffuse * color_diffuse + specular * color_specular, vec4(1.0));

    float shadow = 0.0;
    if (u_enableShadow) {
        shadow = computeShadow(vs_normal, vLightSpacePos);
    }
    out_color.rgb *= (1.0 - 0.7 * shadow);

    float smoke = 0.0;
    if (u_smokeParams.y > u_smokeParams.x && u_smokeParams.x >= 0.0) {
        float dist = length(u_cameraPos - vs_worldpos);
        smoke = clamp((dist - u_smokeParams.x) /
                          max(u_smokeParams.y - u_smokeParams.x, 0.0001),
                      0.0, 1.0);
    }

    if (!smokeEnabled) smoke = 0.0;
    out_color.rgb = mix(out_color.rgb, vec3(1.0), smoke);

    out_color.a = 0.8; // Make it slightly transparent
}
