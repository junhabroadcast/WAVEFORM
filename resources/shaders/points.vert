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
uniform int uComponentPass;   // which component this draw maps (0=Y,1=Cb,2=Cr)
uniform int uLineMode;        // 0 = isolated points, 1 = connected segments (GL_LINES)
uniform vec2 uTraceCenter;    // NDC center of the graticule (vector/lightning)
uniform vec2 uTraceScale;     // NDC units per normalized amplitude (vector/lightning)

out float vIntensity;

float normY(float y10) { return (y10 - 64.0) / 876.0; }
float normC(float c10) { return (c10 - 512.0) / 896.0; }

void discardVertex() {
    gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
    gl_PointSize = 1.0;
    vIntensity = 0.0;
}

void main() {
    int width = uSize.x;
    int height = uSize.y;
    int id = gl_VertexID;

    // Resolve which video sample this vertex maps to.
    // Line mode: vertices come in pairs (seg, seg+1) so consecutive samples
    // are connected, tracing the signal like a real CRT vectorscope.
    int sampleIdx;
    if (uLineMode == 1) {
        int seg = id >> 1;
        sampleIdx = seg + (id & 1);
        if ((seg % width) == (width - 1)) {
            // Segment would wrap from end of one video line to start of next.
            discardVertex();
            return;
        }
    } else {
        sampleIdx = id;
    }

    int x;
    int y;
    if (uLineSelect >= 0) {
        y = clamp(uLineSelect, 0, height - 1);
        x = sampleIdx;
        if (x >= width) {
            discardVertex();
            return;
        }
    } else {
        x = sampleIdx % width;
        y = sampleIdx / width;
        if (y >= height) {
            discardVertex();
            return;
        }
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
        pos = vec2(xn * 2.0 - 1.0, yn * 2.0 - 1.0);
        pos.y = (yn * 0.84 + 0.08) * 2.0 - 1.0;
    } else if (uMode == 1) {
        // Vector: Cb x, Cr y — same center/scale as the graticule circle,
        // so 100% saturation lands exactly on the circle and bars in the boxes.
        vec2 q = vec2(normC(Cb), normC(Cr)) * uGain;
        pos = uTraceCenter + q * uTraceScale;
    } else {
        // Lightning: upper half Pb vs Y, lower half Pr vs Y (mirrored),
        // matching the graticule target mapping exactly.
        float yv = clamp(normY(Y), 0.0, 1.0);
        if (uComponentPass == 0) {
            float pb = normC(Cb) * uGain;
            pos = vec2(uTraceCenter.x + pb * uTraceScale.x,
                       uTraceCenter.y + yv * uTraceScale.y);
        } else {
            float pr = normC(Cr) * uGain;
            pos = vec2(uTraceCenter.x + pr * uTraceScale.x,
                       uTraceCenter.y - (1.0 - yv) * uTraceScale.y);
        }
    }

    // Component enable filter for waveform
    if (uMode == 0) {
        int bit = 1 << uComponentPass;
        if ((uComponents & bit) == 0) {
            discardVertex();
            return;
        }
    }

    gl_Position = vec4(pos, 0.0, 1.0);
    gl_PointSize = (uMode == 0) ? 1.0 : 2.0;
    vIntensity = 1.0;
}
