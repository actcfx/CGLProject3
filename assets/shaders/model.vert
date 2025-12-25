#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform vec4 uClipPlane;
uniform mat4 uLightSpace;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vWorldPos;
out vec4 vLightSpacePos;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vTexCoord = aTexCoord;

    vLightSpacePos = uLightSpace * worldPos;

    vec4 eyePos = uView * worldPos;
    gl_ClipDistance[0] = dot(uClipPlane, eyePos);

    gl_Position = uProjection * eyePos;
}
