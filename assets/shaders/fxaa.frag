#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float width;
uniform float height;

// FXAA tuning (reasonable defaults set from C++)
uniform float spanMax;     // e.g. 8.0
uniform float reduceMin;   // e.g. 1.0/128.0
uniform float reduceMul;   // e.g. 1.0/8.0

float luma(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec2 invRes = vec2(1.0 / max(width, 1.0), 1.0 / max(height, 1.0));

    vec3 rgbM  = texture(screenTexture, TexCoords).rgb;
    vec3 rgbNW = texture(screenTexture, TexCoords + invRes * vec2(-1.0,  1.0)).rgb;
    vec3 rgbNE = texture(screenTexture, TexCoords + invRes * vec2( 1.0,  1.0)).rgb;
    vec3 rgbSW = texture(screenTexture, TexCoords + invRes * vec2(-1.0, -1.0)).rgb;
    vec3 rgbSE = texture(screenTexture, TexCoords + invRes * vec2( 1.0, -1.0)).rgb;

    float lumaM  = luma(rgbM);
    float lumaNW = luma(rgbNW);
    float lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW);
    float lumaSE = luma(rgbSE);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * reduceMul), reduceMin);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = clamp(dir * rcpDirMin, vec2(-spanMax), vec2(spanMax)) * invRes;

    vec3 rgbA = 0.5 * (
        texture(screenTexture, TexCoords + dir * (1.0/3.0 - 0.5)).rgb +
        texture(screenTexture, TexCoords + dir * (2.0/3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(screenTexture, TexCoords + dir * (-0.5)).rgb +
        texture(screenTexture, TexCoords + dir * ( 0.5)).rgb);

    float lumaB = luma(rgbB);

    vec3 result = (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
    FragColor = vec4(result, 1.0);
}
