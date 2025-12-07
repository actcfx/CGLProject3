#version 430 core
layout (location = 0) out vec4 out_color;
in vec3 vs_worldpos;
in vec3 vs_normal;
in vec2 vs_texcoord;

uniform vec4 color_ambient = vec4(0.1, 0.2, 0.5, 1.0);
uniform vec4 color_diffuse = vec4(0.0, 0.2, 0.7, 1.0); // Blue
uniform vec4 color_specular = vec4(1.0, 1.0, 1.0, 1.0);
uniform vec4 color = vec4(0.0, 0.2, 0.7, 1.0); // Blue
uniform float shininess = 50.0f;
uniform vec3 light_position = vec3(50.0f, 32.0f, 560.0f);
uniform vec3 u_cameraPos;

uniform sampler2D u_texture;
uniform float u_time;
uniform vec2 u_scroll;

void main(void){
    vec3 light_direction = normalize(light_position - vs_worldpos);
    vec3 view_direction = normalize(u_cameraPos - vs_worldpos);
    vec3 half_vector = normalize(light_direction + view_direction);

    float diffuse = max(0.0, dot(vs_normal, light_direction));
    float specular = pow(max(0.0, dot(vs_normal, half_vector)), shininess);

    // Add some texture variation to the color for "caustics" look
    vec2 uv1 = vs_texcoord + u_time * u_scroll;
    vec2 uv2 = vs_texcoord - u_time * u_scroll * 0.5;
    float h1 = texture(u_texture, uv1).r;
    float h2 = texture(u_texture, uv2).r;
    float noise = (h1 + h2) * 0.5;

    vec4 finalColor = color * (0.8 + 0.4 * noise); // Modulate color by noise

    out_color = min(finalColor * color_ambient + diffuse * color_diffuse + specular * color_specular, vec4(1.0));
    out_color.a = 0.8;
}