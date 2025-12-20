#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float width;
uniform float height;

const float colorLevels = 6.0;
const float edgeThreshold = 0.2;
const float brushRadius = 2.0;

float random(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 texel = vec2(1.0 / width, 1.0 / height);

    // Soft blur with a little jitter to mimic brush strokes
    vec2 offsets[9] = vec2[](vec2(-1.0, -1.0), vec2(0.0, -1.0),
                             vec2(1.0, -1.0), vec2(-1.0, 0.0),
                             vec2(0.0, 0.0), vec2(1.0, 0.0),
                             vec2(-1.0, 1.0), vec2(0.0, 1.0),
                             vec2(1.0, 1.0));
    float weights[9] = float[](1.0, 2.0, 1.0,
                               2.0, 4.0, 2.0,
                               1.0, 2.0, 1.0);

    vec3 blurred = vec3(0.0);
    float weightSum = 0.0;
    for (int i = 0; i < 9; ++i) {
        vec2 jitter = texel * (random(TexCoords * (12.0 + float(i))) - 0.5) *
                      brushRadius;
        vec2 sampleUV = TexCoords + offsets[i] * texel * brushRadius + jitter;
        vec3 sampleColor = texture(screenTexture, sampleUV).rgb;
        blurred += sampleColor * weights[i];
        weightSum += weights[i];
    }
    blurred /= weightSum;

    // Quantize colors to flatten shading
    vec3 quantized = floor(blurred * colorLevels) / colorLevels;

    // Edge detection on luminance
    float luminance[9];
    for (int i = 0; i < 9; ++i) {
        vec3 c = texture(screenTexture, TexCoords + offsets[i] * texel).rgb;
        luminance[i] = dot(c, vec3(0.299, 0.587, 0.114));
    }
    float gx = luminance[2] + 2.0 * luminance[5] + luminance[8] -
               (luminance[0] + 2.0 * luminance[3] + luminance[6]);
    float gy = luminance[6] + 2.0 * luminance[7] + luminance[8] -
               (luminance[0] + 2.0 * luminance[1] + luminance[2]);
    float edgeMag = sqrt(gx * gx + gy * gy);
    float edgeMask = smoothstep(edgeThreshold, edgeThreshold + 0.35, edgeMag);

    // Subtle canvas grain
    float grain = (random(gl_FragCoord.xy * 0.75) - 0.5) * 0.08;
    quantized = clamp(quantized + grain, 0.0, 1.0);

    // Gentle vignette to keep focus toward the center
    float dist = length(TexCoords - 0.5) * 1.35;
    float vignette = 1.0 - smoothstep(0.55, 0.95, dist);
    vec3 painted = quantized * vignette;

    // Darken along edges for an inked look
    painted = mix(painted, vec3(0.05), edgeMask);

    FragColor = vec4(painted, 1.0);
}
