#version 330 core
in vec2 vUv;
out vec4 fragColor;
uniform sampler2D uAccum;
uniform float uDecay;

void main() {
    float d = texture(uAccum, vUv).r * uDecay;
    fragColor = vec4(d, 0.0, 0.0, 1.0);
}
