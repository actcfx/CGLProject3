#version 430 core
layout (location = 0) out vec4 out_color;
in vec3 vs_worldpos;
in vec3 vs_normal;

uniform vec4 color_ambient = vec4(0.1, 0.2, 0.5, 1.0);
uniform vec4 color_diffuse = vec4(0.0, 0.2, 0.7, 1.0); // Adjusted to blue
uniform vec4 color_specular = vec4(1.0, 1.0, 1.0, 1.0);
uniform vec4 color = vec4(0.0, 0.2, 0.7, 1.0); // Adjusted to blue
uniform float shininess = 50.0f;
uniform vec3 light_position = vec3(50.0f, 32.0f, 560.0f);
uniform vec3 u_cameraPos;

void main(void){
    vec3 light_direction = normalize(light_position - vs_worldpos);
    vec3 view_direction = normalize(u_cameraPos - vs_worldpos);
    vec3 half_vector = normalize(light_direction + view_direction);

    float diffuse = max(0.0, dot(vs_normal, light_direction));
    float specular = pow(max(0.0, dot(vs_normal, half_vector)), shininess);

    out_color = min(color * color_ambient + diffuse * color_diffuse + specular * color_specular, vec4(1.0));
    out_color.a = 0.8; // Make it slightly transparent
}
