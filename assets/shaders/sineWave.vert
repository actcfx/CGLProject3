#version 430 core
layout (location = 0) in vec3 position; // base mesh position (y=0)
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;
layout (location = 3) in vec3 color_in; // unused base color

uniform mat4 u_model;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
};

// Wave parameters
uniform int u_waveCount;
uniform vec2 u_direction[16];
uniform float u_wavelength[16];
uniform float u_amplitude[16];
uniform float u_speed[16];
uniform float u_time;
uniform vec2 u_scroll; // heightmap scroll velocity
uniform float u_heightScale; // heightmap influence scale
uniform sampler2D u_texture; // height map texture (shared with fragment)

out V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
   vec3 color;
} v_out;

void main()
{
    const float PI = 3.14159265359;

    // Sample height map with scrolling
    vec2 uv = texture_coordinate + u_scroll * u_time;
    float heightMap = texture(u_texture, uv).r * u_heightScale;

    // Sum linear sine waves
    float waveHeight = 0.0;
    for (int i = 0; i < u_waveCount; ++i) {
        float k = 2.0 * PI / u_wavelength[i];        // spatial frequency
        float phi = u_speed[i] * 2.0 * PI / u_wavelength[i]; // temporal frequency
        float d = dot(u_direction[i], position.xz);
        waveHeight += u_amplitude[i] * sin(d * k + u_time * phi);
    }

    float finalHeight = heightMap + waveHeight;
    vec3 displaced = vec3(position.x, finalHeight, position.z);

    gl_Position = u_projection * u_view * u_model * vec4(displaced, 1.0f);

    v_out.position = vec3(u_model * vec4(displaced, 1.0f));
    // Approximate normal as up; (Improvement: derive from gradient)
    v_out.normal = mat3(transpose(inverse(u_model))) * vec3(0.0, 1.0, 0.0);
    v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0f - texture_coordinate.y);
    float blue = 0.6 + finalHeight * 3.0;
    float green = 0.5 + finalHeight * 2.0;
    v_out.color = vec3(0.0, clamp(green, 0.0, 1.0), clamp(blue, 0.0, 1.0));
}