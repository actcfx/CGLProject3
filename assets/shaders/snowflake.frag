#version 430 core
out vec4 f_color;

in V_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 bary;
    vec2 uv;
} f_in;

uniform vec3 u_color = vec3(0.05, 0.05, 0.05);
uniform vec3 u_cameraPos;
uniform vec2 u_smokeParams;
uniform bool smokeEnabled;
uniform float u_time;

float sierpinski(vec3 bary) {
    vec3 b = bary;
    for (int i = 0; i < 9; ++i) {
        if (b.x <= 0.5 && b.y <= 0.5 && b.z <= 0.5) {
            return 0.0;
        }

        if (b.x > 0.5) {
            b = vec3(2.0 * b.x - 1.0, 2.0 * b.y, 2.0 * b.z);
        } else if (b.y > 0.5) {
            b = vec3(2.0 * b.x, 2.0 * b.y - 1.0, 2.0 * b.z);
        } else {
            b = vec3(2.0 * b.x, 2.0 * b.y, 2.0 * b.z - 1.0);
        }
    }
    return 1.0;
}

float outlineMask(vec3 bary) {
    float edge = min(min(bary.x, bary.y), bary.z);
    return smoothstep(0.0, 0.01, edge);
}

void main() {
    vec3 clampedBary = clamp(f_in.bary, 0.0, 1.0);
    float pattern = sierpinski(clampedBary);
    float edge = outlineMask(clampedBary);

    vec3 baseFill = mix(vec3(1.0), u_color, pattern);
    vec3 withOutline = mix(baseFill, vec3(0.0), 1.0 - edge);

    float smoke = 0.0;
    if (smokeEnabled && u_smokeParams.y > u_smokeParams.x && u_smokeParams.x >= 0.0) {
        float dist = length(u_cameraPos - f_in.worldPos);
        smoke = clamp((dist - u_smokeParams.x) /
                          max(u_smokeParams.y - u_smokeParams.x, 0.0001),
                      0.0, 1.0);
    }

    vec3 finalColor = mix(withOutline, vec3(1.0), smoke);
    f_color = vec4(finalColor, 1.0);
}

