#version 430 core
layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;

out vec3 vs_worldpos;
out vec3 vs_normal;

uniform mat4 u_model;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
};

uniform float u_time;
uniform int u_waveCount;
uniform vec2 u_direction[20];
uniform float u_wavelength[20];
uniform float u_amplitude[20];
uniform float u_speed[20];

const float PI = 3.14159265359;

void main(void){
    vec4 pos = position;

    float y = 0.0;
    float dIdx = 0.0;
    float dIdz = 0.0;

    for(int i = 0; i < u_waveCount; ++i) {
        float L = u_wavelength[i];
        float A = u_amplitude[i];
        float S = u_speed[i];
        vec2 D = normalize(u_direction[i]);

        float k = 2.0 * PI / L;
        float phase = S * k;
        float theta = dot(D, pos.xz) * k + u_time * phase;

        y += A * sin(theta);

        // Derivative for normal calculation
        float wa = k * A * cos(theta);
        dIdx += wa * D.x;
        dIdz += wa * D.y;
    }

    pos.y = y;

    // Normal = normalize(-dy/dx, 1, -dy/dz)
    vec3 newNormal = normalize(vec3(-dIdx, 1.0, -dIdz));

    vec4 worldPos = u_model * pos;
    gl_Position = u_projection * u_view * worldPos;
    vs_worldpos = worldPos.xyz;
    vs_normal = normalize(mat3(u_model) * newNormal);
}