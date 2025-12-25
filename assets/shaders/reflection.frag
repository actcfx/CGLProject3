#version 430 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vClipSpace;
in vec4 vLightSpacePos;

uniform sampler2D u_reflectionTex;
uniform sampler2D u_refractionTex;
uniform sampler2D u_depthTex;
uniform samplerCube u_skybox;

uniform vec3 u_cameraPos;
uniform float u_waterHeight;
uniform float u_time;
uniform float u_distortionStrength;
uniform float u_normalStrength;
uniform float u_reflectRefractRatio;
uniform vec3 u_waterColor;
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

// Procedural normal waves for realistic water surface
vec3 computeNormal()
{
    // Multiple wave patterns for more complex surface
    float wave1 = sin((vWorldPos.x + u_time * 0.35) * 1.3) * 0.5;
    float wave2 = cos((vWorldPos.z - u_time * 0.27) * 1.1) * 0.5;
    float wave3 = sin((vWorldPos.x + vWorldPos.z - u_time * 0.22) * 0.9) * 0.3;

    // Combined wave pattern
    vec2 grad = (vec2(wave1, wave2) + vec2(wave3, -wave3)) * u_normalStrength;

    return normalize(vec3(grad.x, 1.0, grad.y));
}

void main()
{
    vec3 normal = computeNormal();
    vec3 viewDir = normalize(u_cameraPos - vWorldPos);

    // Screen-space coordinates for sampling reflection / refraction maps
    vec2 ndc = vClipSpace.xy / vClipSpace.w;
    vec2 baseUV = ndc * 0.5 + 0.5;

    // Dynamic ripple distortion for more realistic waves
    vec2 ripple = vec2(
        sin((vWorldPos.z + u_time * 0.45) * 1.7) * 0.7,
        cos((vWorldPos.x - u_time * 0.33) * 1.9) * 0.7
    ) * u_distortionStrength;

    // Use stronger normal-based distortion for refraction to show underwater objects shifting more
    vec2 normalDistort = normal.xz * 0.25;

    // Ensure UV coordinates stay within valid range [0, 1]
    vec2 reflectUV = clamp(baseUV + ripple, 0.002, 0.998);
    vec2 refractUV = clamp(baseUV - normalDistort - ripple * 1.2, 0.002, 0.998);

    // Sample reflection and refraction from FBOs
    vec4 reflection = texture(u_reflectionTex, reflectUV);
    vec4 refraction = texture(u_refractionTex, refractUV);

    // Fallback to skybox if FBOs are not ready (ensures all objects are visible)
    if (textureSize(u_reflectionTex, 0).x <= 1) {
        reflection = vec4(texture(u_skybox, reflect(-viewDir, normal)).rgb, 1.0);
    }
    if (textureSize(u_refractionTex, 0).x <= 1) {
        refraction = vec4(texture(u_skybox, refract(-viewDir, normal, 1.0 / 1.33)).rgb, 1.0);
    }

    // Slightly tint the reflection with environment lighting
    vec3 skyRef = texture(u_skybox, reflect(-viewDir, normal)).rgb;
    reflection.rgb = mix(reflection.rgb, skyRef, 0.15);

    // Fresnel term for realistic blending
    // Objects appear more reflective at shallow angles (grazing angles)
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 2.5);
    fresnel = clamp(fresnel * 1.0 + 0.1, 0.15, 0.7);

    // Depth-based attenuation to keep shallow areas lighter
    float depthSample = texture(u_depthTex, refractUV).r;
    float depthFade = clamp(1.0 - depthSample, 0.0, 1.0);

    // Balance reflection and refraction
    // Looking at steep angles: more reflection
    // Looking down: more refraction
    float reflectionWeight = fresnel * u_reflectRefractRatio;
    vec3 combined = mix(refraction.rgb, reflection.rgb, reflectionWeight);

    // Add water color for absorption effect
    combined = mix(combined, u_waterColor, 0.06 + depthFade * 0.2);

    float shadow = 0.0;
    if (u_enableShadow) {
        shadow = computeShadow(vNormal, vLightSpacePos);
    }
    combined *= (1.0 - 0.7 * shadow);

    float smoke = 0.0;
    if (u_smokeParams.y > u_smokeParams.x && u_smokeParams.x >= 0.0) {
        float dist = length(u_cameraPos - vWorldPos);
        smoke = clamp((dist - u_smokeParams.x) /
                          max(u_smokeParams.y - u_smokeParams.x, 0.0001),
                      0.0, 1.0);
    }

    if (!smokeEnabled) smoke = 0.0;
    combined = mix(combined, vec3(1.0), smoke);

    // Blend transparency based on fresnel for realistic appearance
    float alpha = mix(0.75, 0.92, fresnel);

    FragColor = vec4(combined, alpha);
}