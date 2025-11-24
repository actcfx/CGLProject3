#version 430 core

out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 ClipSpace;

// Textures
uniform sampler2D u_reflectionTex;
uniform sampler2D u_refractionTex;
uniform sampler2D u_depthTex;
uniform samplerCube u_skybox;

// Uniforms
uniform vec3 u_cameraPos;
uniform float u_waterHeight;

const float waveStrength = 0.02;
const vec3 waterColor = vec3(0.0, 0.3, 0.5);
const float reflectionOffset = 0.1; // Offset to shift reflection upward (reduced)

void main()
{
    // View direction and normal
    vec3 viewDir = normalize(u_cameraPos - WorldPos);
    vec3 normal = normalize(Normal);

    // Calculate reflection direction for skybox sampling
    vec3 reflectDir = reflect(-viewDir, normal);

    // Shift reflection upward by adjusting Y component
    reflectDir.y += reflectionOffset;
    reflectDir = normalize(reflectDir);

    // Sample skybox with reflected direction
    vec4 reflectColor = texture(u_skybox, reflectDir);

    // If skybox sampling failed, use a default color
    if (reflectColor.rgb == vec3(0.0)) {
        reflectColor = vec4(0.5, 0.7, 1.0, 1.0);
    }

    // Get refraction from below water (actual scene)
    vec2 ndc = (ClipSpace.xy / ClipSpace.w) / 2.0 + 0.5;
    vec2 refractTexCoords = vec2(ndc.x, ndc.y);

    // Add slight distortion
    vec2 distortion = normal.xz * waveStrength;
    refractTexCoords = clamp(refractTexCoords + distortion, 0.001, 0.999);

    // Sample what's below the water from refraction texture
    vec4 refractColor = texture(u_refractionTex, refractTexCoords);

    // If no refraction texture, use skybox as fallback
    if (refractColor.rgb == vec3(0.0) || textureSize(u_refractionTex, 0).x <= 1) {
        vec3 refractDir = refract(-viewDir, normal, 0.75);
        if (length(refractDir) < 0.01) {
            refractDir = reflectDir;
        }
        refractColor = texture(u_skybox, refractDir);
        if (refractColor.rgb == vec3(0.0)) {
            refractColor = vec4(waterColor, 1.0);
        }
    }

    // Fresnel effect (more reflection at grazing angles)
    float fresnel = dot(viewDir, normal);
    fresnel = pow(1.0 - fresnel, 2.0);
    fresnel = clamp(fresnel, 0.2, 0.8);

    // Mix reflection and refraction based on Fresnel
    vec4 finalColor = mix(refractColor, reflectColor, fresnel);

    // Add water tint (reduced for more transparency)
    finalColor = mix(finalColor, vec4(waterColor, 1.0), 0.03);

    // Very transparent water to see through
    FragColor = vec4(finalColor.rgb, 0.8);
}