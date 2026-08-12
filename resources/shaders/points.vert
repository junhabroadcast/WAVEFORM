#version 330 core

uniform sampler2D uY;
uniform sampler2D uCb;
uniform sampler2D uCr;
uniform ivec2 uSize;          // width, height
uniform int uMode;            // 0 waveform, 1 vector, 2 lightning
uniform int uStyle;           // 0 overlay, 1 parade (waveform)
uniform int uSweep;           // 0 line, 1 field
uniform int uComponents;      // bit0 Y bit1 Cb bit2 Cr
uniform float uGain;
uniform float uMag;
uniform int uLineSelect;      // -1 disabled, else line index
uniform int uComponentPass;   // which component this draw maps (0=Y,1=Cb,2=Cr) for parade/overlay filter

out float vIntensity;

float normY(float y10) { return (y10 - 64.0) / 876.0; }
float normC(float c10) { return (c10 - 512.0) / 896.0; }

void main() {
    int width = uSize.x;
    int height = uSize.y;
    int id = gl_VertexID;
    int x = id % width;
    int y = id / width;

    if (uLineSelect >= 0) {
        y = clamp(uLineSelect, 0, height - 1);
        // replicate selected line across vertical ids by ignoring y from id
        x = id % width;
        // only first width vertices matter; others discarded by putting offscreen
        if (id >= width) {
            gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
            vIntensity = 0.0;
            return;
        }
    } else if (uSweep == 0) {
        // Line sweep: stack all lines onto one horizontal trace (classic WFM)
        // x from sample, y from amplitude only — line index ignored for X
    }

    vec2 uv = (vec2(float(x) + 0.5, float(y) + 0.5) / vec2(float(width), float(height)));
    float Y = texture(uY, uv).r * 65535.0;
    float Cb = texture(uCb, uv).r * 65535.0;
    float Cr = texture(uCr, uv).r * 65535.0;

    float amp = 0.0;
    if (uComponentPass == 0) amp = normY(Y);
    else if (uComponentPass == 1) amp = normC(Cb) + 0.5;
    else amp = normC(Cr) + 0.5;

    amp = (amp - 0.5) * uGain + 0.5;

    vec2 pos = vec2(0.0);
    if (uMode == 0) {
        // Waveform
        float xn = float(x) / float(max(width - 1, 1));
        if (uMag > 1.0) {
            xn = (xn - 0.5) * uMag + 0.5;
        }
        if (uStyle == 1) {
            // Parade: three horizontal bands
            xn = (xn + float(uComponentPass)) / 3.0;
        }
        float yn = amp;
        // NDC
        pos = vec2(xn * 2.0 - 1.0, yn * 2.0 - 1.0);
        // Keep vertical headroom
        pos.y = (yn * 0.84 + 0.08) * 2.0 - 1.0;
    } else if (uMode == 1) {
        // Vector: Cb x, Cr y
        float cx = normC(Cb) * uGain;
        float cy = normC(Cr) * uGain;
        pos = vec2(cx * 2.0, cy * 2.0);
    } else {
        // Lightning: upper Pb vs Y, lower Pr vs Y — emit two points via pass
        float yv = clamp(normY(Y), 0.0, 1.0);
        if (uComponentPass == 0) {
            float pb = normC(Cb) * uGain;
            pos = vec2(pb * 1.8, yv);           // upper half mapped later
            pos.y = pos.y * 0.5 + 0.5;          // 0.5..1.0 -> top
            pos = pos * 2.0 - 1.0;
        } else {
            float pr = normC(Cr) * uGain;
            pos = vec2(pr * 1.8, 1.0 - yv);
            pos.y = pos.y * 0.5;                // 0..0.5 -> bottom
            pos = pos * 2.0 - 1.0;
        }
    }

    // Component enable filter for waveform
    if (uMode == 0) {
        int bit = 1 << uComponentPass;
        if ((uComponents & bit) == 0) {
            gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
            vIntensity = 0.0;
            return;
        }
    }

    gl_Position = vec4(pos, 0.0, 1.0);
    gl_PointSize = 1.0;
    vIntensity = 1.0;
}
