#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float Height;

uniform vec3 u_viewPos;
uniform vec3 u_lightDir;

void main()
{
    // Simple lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_lightDir);
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 ambient = 0.1 * vec3(1.0);
    vec3 diffuse = diff * vec3(1.0);

    // Color based on height
    vec3 color;
    if (Height < 20.0) {
        color = vec3(0.2, 0.6, 0.2); // Green grass
    } else if (Height < 100.0) {
        float t = (Height - 20.0) / 80.0;
        color = mix(vec3(0.2, 0.6, 0.2), vec3(0.5, 0.4, 0.3), t); // Grass to dirt
    } else {
        float t = clamp((Height - 10.0) / 10.0, 0.0, 1.0);
        color = mix(vec3(0.5, 0.4, 0.3), vec3(0.9, 0.9, 0.9), t); // Dirt to snow/rock
    }

    vec3 result = (ambient + diffuse) * color;
    FragColor = vec4(result, 1.0);
}
