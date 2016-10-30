#ifdef GL_ES
precision mediump float;
#endif

varying vec2 texCoord;

uniform sampler2D tex;

void kore() {
    gl_FragColor = texture2D(tex, texCoord);
}
