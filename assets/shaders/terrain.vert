#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 color;
    vec4 lightSpacePos;
} vs_out;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_lightSpace;
uniform vec4 u_clipPlane;
uniform bool u_enableClip;

void main() {
    vec4 world = u_model * vec4(aPos, 1.0);
    vs_out.worldPos = world.xyz;

    mat3 normalMat = transpose(inverse(mat3(u_model)));
    vs_out.normal = normalize(normalMat * aNormal);
    vs_out.color = aColor;
    vs_out.lightSpacePos = u_lightSpace * world;

    if (u_enableClip) {
        gl_ClipDistance[0] = dot(world, u_clipPlane);
    } else {
        gl_ClipDistance[0] = 1.0;
    }

    gl_Position = u_proj * u_view * world;
}
