#version 330 compatibility

out vec3 v_posEye;
out vec3 v_normalEye;
out vec2 v_uv;
out vec4 v_color;

void main() {
    vec4 posEye4 = gl_ModelViewMatrix * gl_Vertex;
    v_posEye = posEye4.xyz;

    v_normalEye = normalize(gl_NormalMatrix * gl_Normal);
    v_uv = gl_MultiTexCoord0.xy;
    v_color = gl_Color;

    gl_Position = gl_ProjectionMatrix * posEye4;
}
