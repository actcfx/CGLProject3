#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float width;
uniform float height;

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

float hatch(vec2 dir, float spacing, float thickness) {
    // Project the fragment coordinate onto a direction and wrap to [0,1)
    float coord = dot(gl_FragCoord.xy, normalize(dir)) / spacing;
    float f = fract(coord);
    float dist = abs(f - 0.5);
    // thickness expressed as a fraction of half the period (0..0.5)
    return 1.0 - smoothstep(0.5 - thickness, 0.5, dist);
}

void main() {
    vec2 texel = vec2(1.0 / width, 1.0 / height);

    // Base color and luminance
    vec3 baseColor = texture(screenTexture, TexCoords).rgb;
    float shade = luminance(baseColor);

    // Sobel edge detection on luminance
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
    float edgeMask = smoothstep(0.12, 0.35, edge);

    // Quantize to flatten tones
    vec3 quantized = floor(baseColor * 6.0) / 6.0;

    // Layered cross-hatching based on tone
    float ink = 1.0;
    if (shade < 0.85) ink = min(ink, hatch(vec2(1.0, 1.0), 12.0, 0.12));
    if (shade < 0.65) ink = min(ink, hatch(vec2(1.0, -1.0), 10.0, 0.12));
    if (shade < 0.50) ink = min(ink, hatch(vec2(0.0, 1.0), 8.0, 0.10));
    if (shade < 0.35) ink = min(ink, hatch(vec2(1.0, 0.0), 6.0, 0.08));

    vec3 inked = quantized * ink;

    // Dark outlines from edge detector
    inked = mix(inked, vec3(0.02), edgeMask);

    // Subtle paper tint
    inked *= vec3(0.98, 0.97, 0.95);

    FragColor = vec4(clamp(inked, 0.0, 1.0), 1.0);
}
