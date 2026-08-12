#version 330 core
in vec2 vUv;
out vec4 fragColor;
uniform sampler2D uAccum;
uniform float uIntensity;

void main() {
    float d = texture(uAccum, vUv).r;
    // CRT-like green phosphor response
    float g = 1.0 - exp(-d * uIntensity);
    float r = g * 0.15;
    float b = g * 0.25;
    fragColor = vec4(r, g, b, 1.0);
}
