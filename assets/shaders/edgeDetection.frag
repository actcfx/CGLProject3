#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float width;
uniform float height;

// Multiplies Sobel magnitude before thresholding
uniform float strength;

// Edge threshold in [0, 1] (tune from C++)
uniform float threshold;

float luminance(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec2 texel = vec2(1.0 / max(width, 1.0), 1.0 / max(height, 1.0));

    float tl = luminance(texture(screenTexture, TexCoords + texel * vec2(-1.0,  1.0)).rgb);
    float tc = luminance(texture(screenTexture, TexCoords + texel * vec2( 0.0,  1.0)).rgb);
    float tr = luminance(texture(screenTexture, TexCoords + texel * vec2( 1.0,  1.0)).rgb);

    float ml = luminance(texture(screenTexture, TexCoords + texel * vec2(-1.0,  0.0)).rgb);
    float mc = luminance(texture(screenTexture, TexCoords + texel * vec2( 0.0,  0.0)).rgb);
    float mr = luminance(texture(screenTexture, TexCoords + texel * vec2( 1.0,  0.0)).rgb);

    float bl = luminance(texture(screenTexture, TexCoords + texel * vec2(-1.0, -1.0)).rgb);
    float bc = luminance(texture(screenTexture, TexCoords + texel * vec2( 0.0, -1.0)).rgb);
    float br = luminance(texture(screenTexture, TexCoords + texel * vec2( 1.0, -1.0)).rgb);

    float gx = (-1.0 * tl) + ( 1.0 * tr)
             + (-2.0 * ml) + ( 2.0 * mr)
             + (-1.0 * bl) + ( 1.0 * br);

    float gy = (-1.0 * tl) + (-2.0 * tc) + (-1.0 * tr)
             + ( 1.0 * bl) + ( 2.0 * bc) + ( 1.0 * br);

    float mag = length(vec2(gx, gy)) * max(strength, 0.0);

    // Convert to a clean edge mask (white edges on black)
    float edge = smoothstep(threshold, threshold * 2.0, mag);

    FragColor = vec4(vec3(edge), 1.0);
}
