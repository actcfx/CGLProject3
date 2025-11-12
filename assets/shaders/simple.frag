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
    vec3 finalColor = mix(texColor, f_in.color, 0.5);
    f_color = vec4(finalColor, 1.0f);

    // f_color = vec4(f_in.color, 1.0f);
}