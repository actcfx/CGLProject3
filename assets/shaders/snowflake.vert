#version 430 core
// Fullscreen triangle, no inputs needed
out V_OUT {
    vec2 uv;
    vec3 worldPos;
} v_out;

void main() {
    // Fullscreen triangle positions
    const vec2 pos[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vec2 p = pos[gl_VertexID];
    gl_Position = vec4(p, 0.0, 1.0);
    v_out.uv = p * 0.5 + 0.5;
    v_out.worldPos = vec3(0.0);
}
