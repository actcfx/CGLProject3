#version 330 compatibility

in vec3 v_posEye;
in vec3 v_normalEye;
in vec2 v_uv;
in vec4 v_color;

in vec3 v_worldNormal;
in vec4 v_lightSpacePos;

uniform sampler2D u_bumpTex;
uniform int u_bumpEnabled;
uniform float u_bumpStrength;

uniform int u_enableLight0;
uniform int u_enableLight1;
uniform int u_enableLight2;

uniform sampler2D u_shadowMap;
uniform vec3 u_lightDir;
uniform bool u_enableShadow;

out vec4 fragColor;

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

float heightAt(vec2 uv) {
    return texture(u_bumpTex, uv).r;
}

vec3 computeBumpNormal(vec3 N) {
    ivec2 texSize = textureSize(u_bumpTex, 0);
    vec2 texel = 1.0 / vec2(max(texSize.x, 1), max(texSize.y, 1));

    float hL = heightAt(v_uv - vec2(texel.x, 0.0));
    float hR = heightAt(v_uv + vec2(texel.x, 0.0));
    float hD = heightAt(v_uv - vec2(0.0, texel.y));
    float hU = heightAt(v_uv + vec2(0.0, texel.y));

    float dHdU = (hR - hL);
    float dHdV = (hU - hD);

    vec3 dp1 = dFdx(v_posEye);
    vec3 dp2 = dFdy(v_posEye);
    vec2 duv1 = dFdx(v_uv);
    vec2 duv2 = dFdy(v_uv);

    vec3 T = normalize(dp1 * duv2.y - dp2 * duv1.y);
    vec3 B = normalize(-dp1 * duv2.x + dp2 * duv1.x);

    vec3 nTS = normalize(vec3(-dHdU * u_bumpStrength, -dHdV * u_bumpStrength, 1.0));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * nTS);
}

vec3 applyLight(int idx, vec3 N, vec3 albedo) {
    vec4 lp = gl_LightSource[idx].position; // eye space
    vec3 L = (lp.w == 0.0) ? normalize(lp.xyz) : normalize(lp.xyz - v_posEye);

    vec3 V = normalize(-v_posEye);
    vec3 H = normalize(L + V);

    float ndotl = max(dot(N, L), 0.0);
    float specPow = 32.0;
    float spec = pow(max(dot(N, H), 0.0), specPow);

    vec3 ambient = gl_LightSource[idx].ambient.rgb * albedo;
    vec3 diffuse = gl_LightSource[idx].diffuse.rgb * ndotl * albedo;
    vec3 specular = gl_LightSource[idx].specular.rgb * spec;

    return ambient + diffuse + specular;
}

void main() {
    vec3 albedo = v_color.rgb;
    vec3 N = normalize(v_normalEye);

    if (u_bumpEnabled != 0) {
        N = computeBumpNormal(N);
    }

    vec3 color = vec3(0.0);
    if (u_enableLight0 != 0) color += applyLight(0, N, albedo);
    if (u_enableLight1 != 0) color += applyLight(1, N, albedo);
    if (u_enableLight2 != 0) color += applyLight(2, N, albedo);

    if (u_enableLight0 == 0 && u_enableLight1 == 0 && u_enableLight2 == 0) {
        vec3 L = normalize(vec3(0.3, 0.8, 0.6));
        float ndotl = max(dot(N, L), 0.0);
        color = albedo * (0.2 + 0.8 * ndotl);
    }

    float shadow = 0.0;
    if (u_enableShadow) {
        shadow = computeShadow(v_worldNormal, v_lightSpacePos);
    }

    color *= (1.0 - 0.7 * shadow);
    fragColor = vec4(color, v_color.a);
}
