#version 330 compatibility

out vec3 v_posEye;
out vec3 v_normalEye;
out vec2 v_uv;
out vec4 v_color;

out vec3 v_worldNormal;
out vec4 v_lightSpacePos;

uniform mat4 u_invView;
uniform mat4 uLightSpace;

void main() {
    vec4 posEye4 = gl_ModelViewMatrix * gl_Vertex;
    v_posEye = posEye4.xyz;

    v_normalEye = normalize(gl_NormalMatrix * gl_Normal);
    v_uv = gl_MultiTexCoord0.xy;
    v_color = gl_Color;

    // Reconstruct world position from eye space: world = invView * eye
    vec4 worldPos = u_invView * posEye4;
    v_worldNormal = normalize(mat3(u_invView) * v_normalEye);
    v_lightSpacePos = uLightSpace * worldPos;

    gl_Position = gl_ProjectionMatrix * posEye4;
}
