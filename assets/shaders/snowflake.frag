#version 430 core
#version 430 core
out vec4 f_color;

in V_OUT {
    vec2 uv;
    vec3 worldPos;
} f_in;

uniform float u_time;
uniform vec2 u_smokeParams;
uniform bool smokeEnabled;
uniform vec3 u_cameraPos;

// Signed distance to an upright equilateral triangle centered at the origin
float triSdf(vec2 p) {
    const float k = 1.7320508; // sqrt(3)
    p.x = abs(p.x) - 1.0;
    p.y = p.y + 1.0 / k;
    if (p.x + k * p.y > 0.0) p = vec2(p.x - k * p.y, -k * p.x - p.y) * 0.5;
    p.x += 1.0;
    return -max(-p.x, k * p.y);
}

// Small noise to keep lines from banding perfectly
float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

void main() {
    // Center and scale to an equilateral framing
    vec2 uv = f_in.uv - 0.5;
    uv.x *= 1.7320508; // width:height = sqrt(3):1
    uv *= 1.05;

    // Build Sierpinski gasket and edge strokes in simplex space
    vec2 p = uv;
    float mask = 1.0;
    float lineDist = 1e9;
    float scale = 1.0;
    const int ITER = 9;
    for (int i = 0; i < ITER; ++i) {
        p = abs(p);
        if (p.x + p.y > 1.0) p = vec2(1.0) - p;

        float edge = min(min(p.x, p.y), 1.0 - p.x - p.y);
        lineDist = min(lineDist, edge * scale);

        float hole = step(0.333, p.x) * step(0.333, p.y) * (1.0 - step(0.666, p.x + p.y));
        mask *= 1.0 - hole;

        p = p * 2.0 - vec2(1.0);
        scale *= 0.5;
    }

    // Outer silhouette
    float sdf = triSdf(uv * 1.01);
    float outline = smoothstep(0.003, 0.0, sdf);

    // Edge strokes kept constant across levels
    float strokes = smoothstep(0.0035, 0.0, lineDist);

    // Colors: black background, gray strokes
    float jitter = hash11(dot(uv, vec2(157.0, 311.0)) + u_time * 1.5);
    vec3 lineColor = vec3(0.8) + vec3(jitter) * 0.06;
    vec3 background = vec3(0.0);
    float coverage = outline * mask;
    vec3 color = mix(background, lineColor, coverage * strokes);

    // Opaque background so the plane always shows; silhouette clamps where pattern lives
    float alpha = 1.0;

    // Smoke fade like the other shaders
    float smoke = 0.0;
    if (u_smokeParams.y > u_smokeParams.x && u_smokeParams.x >= 0.0) {
        float dist = length(u_cameraPos - f_in.worldPos);
        smoke = clamp((dist - u_smokeParams.x) / max(u_smokeParams.y - u_smokeParams.x, 0.0001), 0.0, 1.0);
    }
    if (!smokeEnabled) smoke = 0.0;
    color = mix(color, vec3(1.0), smoke);
    alpha *= (1.0 - smoke * 0.8);

    f_color = vec4(color, alpha);
}
    }
