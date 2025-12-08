#version 430 core
out vec4 f_color;

in V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
} f_in;

uniform vec3 u_color;
uniform sampler2D u_texture;

void main()
{
    vec4 texColor = texture(u_texture, f_in.texture_coordinate);
    if (texColor.a < 0.05)
        discard;  // drop fully transparent fragments to avoid occluding background
    f_color = texColor;
}
