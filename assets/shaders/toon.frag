#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float width;
uniform float height;

// --- Parameters ---
// Threshold for edge detection (lower = more sensitive edges)
const float edgeThreshold = 2.0;
// Number of color levels for quantization (lower = more "posterized" look)
const float colorLevels = 5.0;

// Helper to calculate luminance
float luminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    // 1. Calculate pixel size for sampling
    float dx = 1.0 / width;
    float dy = 1.0 / height;

    // Sample the 3x3 window around the current pixel
    vec3 t[9];
    int k = 0;
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            t[k] = texture(screenTexture, TexCoords + vec2(float(i)*dx, float(j)*dy)).rgb;
            k++;
        }
    }

    // 2. Sobel Edge Detection
    // Apply Sobel kernels to detect gradients in X and Y directions

    // Kernel X:
    // -1  0  1
    // -2  0  2
    // -1  0  1
    vec3 sobelX = t[2] + 2.0*t[5] + t[8] - (t[0] + 2.0*t[3] + t[6]);

    // Kernel Y:
    // -1 -2 -1
    //  0  0  0
    //  1  2  1
    vec3 sobelY = t[0] + 2.0*t[1] + t[2] - (t[6] + 2.0*t[7] + t[8]);

    // Calculate magnitude of the gradient
    // We use dot product to combine RGB components into a single scalar intensity
    float gradX = dot(sobelX, sobelX);
    float gradY = dot(sobelY, sobelY);
    float gradient = sqrt(gradX + gradY);

    // 3. Color Quantization
    // Use the center pixel (t[4]) as the base color
    vec3 color = t[4];

    // Snap color values to discrete levels (e.g., 0.0, 0.2, 0.4...)
    color = floor(color * colorLevels) / colorLevels;

    // 4. Final Mix
    if (gradient > edgeThreshold) {
        // Draw black outline if gradient exceeds threshold
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // Draw the quantized color
        FragColor = vec4(color, 1.0);
    }
}