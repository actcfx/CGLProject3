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
uniform vec3 u_cameraPos;
uniform vec2 u_smokeParams;

void main()
{   
    vec3 color = vec3(texture(u_texture, f_in.texture_coordinate));
    float smoke = 0.0;
    if (u_smokeParams.y > u_smokeParams.x && u_smokeParams.x >= 0.0) {
        float dist = length(u_cameraPos - f_in.position);
        smoke = clamp((dist - u_smokeParams.x) /
                          max(u_smokeParams.y - u_smokeParams.x, 0.0001),
                      0.0, 1.0);
    }

    vec3 finalColor = mix(color, vec3(1.0), smoke);
    f_color = vec4(finalColor, 1.0f);
}