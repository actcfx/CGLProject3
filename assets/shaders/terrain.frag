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
uniform bool u_enableLight;
// Point light
uniform vec3 u_pointLightPos;
uniform samplerCube u_pointShadowMap;
uniform float u_pointFarPlane;
uniform bool u_enablePointShadow;
uniform bool u_enablePointLight;
// Spot light
uniform vec3 u_spotLightPos;
uniform vec3 u_spotLightDir;
uniform mat4 u_spotLightMatrix;
uniform sampler2D u_spotShadowMap;
uniform float u_spotFarPlane;
uniform float u_spotInnerCos;
uniform float u_spotOuterCos;
uniform bool u_enableSpotShadow;
uniform bool u_enableSpotLight;

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

float computePointShadow(vec3 fragPos) {
    vec3 fragToLight = fragPos - u_pointLightPos;
    float currentDepth = length(fragToLight);
    float bias = 0.006;
    float shadow = 0.0;
    int samples = 20;
    float diskRadius = (1.0 + currentDepth / u_pointFarPlane) * 0.05;
    vec3 sampleOffsetDirections[20] = vec3[](
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  0,  0), vec3(-1,  0,  0), vec3( 0,  1,  0), vec3( 0, -1,  0),
        vec3( 0,  0,  1), vec3( 0,  0, -1), vec3( 1,  1,  0), vec3(-1,  1,  0),
        vec3( 1, -1,  0), vec3(-1, -1,  0), vec3( 1,  0,  1), vec3(-1,  0,  1)
    );

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = normalize(fragToLight + sampleOffsetDirections[i] * diskRadius);
        float closestDepth = texture(u_pointShadowMap, sampleDir).r;
        closestDepth *= u_pointFarPlane;
        shadow += (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
    }
    shadow /= float(samples);
    return shadow;
}

float computeSpotShadow(vec3 fragPos) {
    vec4 lightSpace = u_spotLightMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = lightSpace.xyz / lightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 ||
        projCoords.y > 1.0 || projCoords.z > 1.0)
        return 0.0;

    float currentDepth = length(fragPos - u_spotLightPos);
    float bias = 0.004;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_spotShadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closestDepth =
                texture(u_spotShadowMap,
                        projCoords.xy + vec2(x, y) * texelSize)
                    .r;
            closestDepth *= u_spotFarPlane;
            shadow += (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main() {
    vec3 albedo = fs_in.color;
    vec3 ambient = 0.05 * albedo;

    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(u_viewPos - fs_in.worldPos);

    // Directional light
    vec3 dirLight = vec3(0.0);
    if (u_enableLight) {
        vec3 Ld = normalize(-u_lightDir);
        vec3 Hd = normalize(Ld + V);
        float diffD = max(dot(N, Ld), 0.0);
        float specD = pow(max(dot(N, Hd), 0.0), 32.0) * 0.25;
        float shadowD = (u_enableShadow && u_enableLight)
                            ? computeShadow(N, fs_in.lightSpacePos)
                            : 0.0;
        dirLight = (1.0 - shadowD) * (diffD * albedo + specD);
    }

    // Point light
    vec3 pointLight = vec3(0.0);
    if (u_enablePointLight) {
        vec3 Lp = u_pointLightPos - fs_in.worldPos;
        float dist = length(Lp);
        Lp = normalize(Lp);
        vec3 Hp = normalize(Lp + V);
        float diffP = max(dot(N, Lp), 0.0);
        float specP = pow(max(dot(N, Hp), 0.0), 32.0) * 0.25;
        float attenuation = 1.0 / (1.0 + 0.02 * dist + 0.004 * dist * dist);
        float pointStrength = 1.5;
        float shadowP = (u_enablePointShadow && u_enablePointLight)
                            ? computePointShadow(fs_in.worldPos)
                            : 0.0;
        pointLight = pointStrength * (1.0 - shadowP) * attenuation *
                     (diffP * albedo + specP);
    }

    // Spot light
    vec3 spotLight = vec3(0.0);
    if (u_enableSpotLight) {
        vec3 Ls = u_spotLightPos - fs_in.worldPos;
        float dist = length(Ls);
        Ls = normalize(Ls);
        vec3 Hs = normalize(Ls + V);
        float theta = dot(-u_spotLightDir, Ls);
        float spotEffect = clamp((theta - u_spotOuterCos) /
                                     max(u_spotInnerCos - u_spotOuterCos, 1e-3),
                                 0.0, 1.0);
        if (theta > u_spotOuterCos) {
            float diffS = max(dot(N, Ls), 0.0);
            float specS = pow(max(dot(N, Hs), 0.0), 32.0) * 0.25;
            float attenuation = 1.0 / (1.0 + 0.02 * dist + 0.004 * dist * dist);
            float spotStrength = 1.5;
            float shadowS = (u_enableSpotShadow && u_enableSpotLight)
                                ? computeSpotShadow(fs_in.worldPos)
                                : 0.0;
            spotLight = spotStrength * 1.33 * (1.0 - shadowS) * spotEffect *
                        attenuation * (diffS * albedo + specS);
        }
    }

    vec3 lighting = ambient + dirLight + pointLight + spotLight;
    FragColor = vec4(lighting, 1.0);
}
