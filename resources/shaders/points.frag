#version 330 core
in float vIntensity;
out vec4 fragColor;

uniform float uAdd;

void main() {
    if (vIntensity <= 0.0)
        discard;
    fragColor = vec4(uAdd, 0.0, 0.0, 1.0);
}
