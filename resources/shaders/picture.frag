#version 330 core
in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uY;
uniform sampler2D uCb;
uniform sampler2D uCr;
uniform vec2 uFrameSize;     // video width, height
uniform vec2 uViewportSize;  // widget pixel size
uniform int uColorimetry;    // 0 = BT.601, 1 = BT.709
uniform int uLineSelect;     // -1 off, else 0-based line

float normY(float y10) { return clamp((y10 - 64.0) / 876.0, 0.0, 1.0); }
float normC(float c10) { return (c10 - 512.0) / 896.0; }

void main() {
    // Letterbox / pillarbox to preserve aspect ratio
    float frameAspect = uFrameSize.x / max(uFrameSize.y, 1.0);
    float viewAspect = uViewportSize.x / max(uViewportSize.y, 1.0);
    vec2 uv = vUv;
    // Flip Y: texture row0 is top of picture
    uv.y = 1.0 - uv.y;

    if (viewAspect > frameAspect) {
        float w = frameAspect / viewAspect;
        uv.x = (uv.x - 0.5) / w + 0.5;
    } else {
        float h = viewAspect / frameAspect;
        uv.y = (uv.y - 0.5) / h + 0.5;
    }

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        fragColor = vec4(0.02, 0.02, 0.02, 1.0);
        return;
    }

    float Y = texture(uY, uv).r * 65535.0;
    float Cb = texture(uCb, uv).r * 65535.0;
    float Cr = texture(uCr, uv).r * 65535.0;

    float y = normY(Y);
    float pb = normC(Cb);
    float pr = normC(Cr);

    // Studio range YCbCr -> RGB (approximate BT.601 / BT.709)
    float r, g, b;
    if (uColorimetry == 0) {
        // BT.601
        r = y + 1.402 * pr;
        g = y - 0.344136 * pb - 0.714136 * pr;
        b = y + 1.772 * pb;
    } else {
        // BT.709
        r = y + 1.5748 * pr;
        g = y - 0.1873 * pb - 0.4681 * pr;
        b = y + 1.8556 * pb;
    }

    vec3 rgb = clamp(vec3(r, g, b), 0.0, 1.0);

    // Optional line-select highlighter
    if (uLineSelect >= 0) {
        float lineUv = (float(uLineSelect) + 0.5) / max(uFrameSize.y, 1.0);
        if (abs(uv.y - lineUv) < (1.5 / max(uFrameSize.y, 1.0))) {
            rgb = mix(rgb, vec3(1.0, 0.85, 0.2), 0.65);
        }
    }

    fragColor = vec4(rgb, 1.0);
}
