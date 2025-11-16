#version 430 core
out vec4 f_color;

in V_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texture_coordinate;
    vec3 color;
} f_in;

uniform vec3 u_color;

uniform sampler2D u_texture;

void main()
{
    // Multiply texture color and vertex color
    // vec3 texColor = vec3(texture(u_texture, f_in.texture_coordinate));
    // vec3 finalColor = texColor * f_in.color;
    // f_color = vec4(finalColor, 1.0f);

    // Mix texture color and vertex color
    vec3 texColor = vec3(texture(u_texture, f_in.texture_coordinate));

    // Water shading tweak: keep vertex water tint dominant, texture as detail
    vec3 waterTint = f_in.color;
    // vec3 mixed = mix(waterTint, texColor, 0.2);

    vec3 mixed = f_in.color;

    // Gentle lighting using the provided normal
    vec3 N = normalize(f_in.normal);
    vec3 L = normalize(vec3(0.4, 0.8, 0.4));
    float lambert = clamp(dot(N, L), 0.0, 1.0);
    float highlight = pow(lambert, 8.0) * 0.15;
    vec3 lit = mixed * (0.65 + 0.35 * lambert) + highlight;

    f_color = vec4(lit, 1.0f);

    // f_color = vec4(f_in.color, 1.0f);
}