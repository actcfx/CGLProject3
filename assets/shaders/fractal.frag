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

// Simple smooth palette
vec3 palette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 0.5, 0.0);
    vec3 d = vec3(0.00, 0.33, 0.67);
    return a + b * cos(6.28318 * (c * t + d));
}

void main() {
    // Map UV to complex plane with slight time-based zoom/pan
    vec2 p = (f_in.uv - 0.5) * 3.0;
    float zoom = 0.85 + 0.15 * sin(u_time * 0.2);
    p *= zoom;
    p += vec2(0.1 * sin(u_time * 0.13), 0.1 * cos(u_time * 0.17));

    vec2 z = vec2(0.0);
    const int ITER = 80;
    int i;
    for (i = 0; i < ITER; ++i) {
        // z = z^2 + p (Mandelbrot)
        vec2 z2 = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y);
        z = z2 + p;
        if (dot(z, z) > 8.0) break;
    }

    float t = float(i) / float(ITER);
    vec3 col = palette(t);

    // Soft smoke fade with distance like other shaders
    float smoke = 0.0;
    if (u_smokeParams.y > u_smokeParams.x && u_smokeParams.x >= 0.0) {
        float dist = length(u_cameraPos - f_in.worldPos);
        smoke = clamp((dist - u_smokeParams.x) /
                          max(u_smokeParams.y - u_smokeParams.x, 0.0001),
                      0.0, 1.0);
    }
    if (!smokeEnabled) smoke = 0.0;
    col = mix(col, vec3(1.0), smoke);

    f_color = vec4(col, 1.0);
}
