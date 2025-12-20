#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float width;
uniform float height;

float luminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

float hash21(vec2 p) {
    // simple, fast hash for stipple noise
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

void main() {
    vec2 texel = vec2(1.0 / width, 1.0 / height);

    // Base sample
    vec3 base = texture(screenTexture, TexCoords).rgb;
    float lum = luminance(base);

    // Quantize color slightly to keep a painterly look
    vec3 quant = floor(base * 7.0) / 7.0;

    // Sobel edge detect on luminance
    float l[9];
    int k = 0;
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            vec3 c = texture(screenTexture, TexCoords + vec2(i, j) * texel).rgb;
            l[k++] = luminance(c);
        }
    }
    float gx = l[2] + 2.0 * l[5] + l[8] - (l[0] + 2.0 * l[3] + l[6]);
    float gy = l[0] + 2.0 * l[1] + l[2] - (l[6] + 2.0 * l[7] + l[8]);
    float edge = sqrt(gx * gx + gy * gy);
    float edgeMask = smoothstep(0.10, 0.30, edge);

    // Stipple density increases as luminance goes down
    float density = clamp(1.1 - lum * 1.1, 0.0, 1.0);

    // Coarse jittered grid to avoid moire; scale controls dot spacing
    float gridScale = 3.5; // smaller = denser grid
    vec2 cell = floor(gl_FragCoord.xy * gridScale);
    float n = hash21(cell);
    float dotMask = step(n, density);

    // Blend dots: darken where dots appear
    vec3 dotted = mix(quant, quant * 0.25, dotMask * 0.8);

    // Keep strong outlines
    dotted = mix(dotted, vec3(0.02), edgeMask);

    // Mild paper tint
    dotted *= vec3(0.99, 0.985, 0.97);

    FragColor = vec4(clamp(dotted, 0.0, 1.0), 1.0);
}
