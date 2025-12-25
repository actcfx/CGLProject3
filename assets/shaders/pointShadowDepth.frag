#version 330 core
in vec3 vWorldPos;

uniform vec3 u_lightPos;
uniform float u_farPlane;

void main() {
    float lightDist = length(vWorldPos - u_lightPos);
    lightDist = lightDist / u_farPlane;
    gl_FragDepth = lightDist;
}
