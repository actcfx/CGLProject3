#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 color;
    vec4 lightSpacePos;
} fs_in;

uniform vec3 u_lightDir;
uniform vec3 u_viewPos;
uniform sampler2D u_shadowMap;
uniform bool u_enableShadow;

float computeShadow(vec3 normal, vec4 lightSpacePos) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside the light frustum: no shadow contribution
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
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main() {
    vec3 N = normalize(fs_in.normal);
    vec3 L = normalize(-u_lightDir);
    vec3 V = normalize(u_viewPos - fs_in.worldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.25;

    float shadow = u_enableShadow ? computeShadow(N, fs_in.lightSpacePos) : 0.0;

    vec3 albedo = fs_in.color;
    vec3 ambient = 0.15 * albedo;
    vec3 lighting = ambient + (1.0 - shadow) * (diff * albedo + spec);

    FragColor = vec4(lighting, 1.0);
}
