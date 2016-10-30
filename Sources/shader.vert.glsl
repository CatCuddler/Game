#ifdef GL_ES
precision highp float;
#endif

attribute vec3 pos;
attribute vec2 tex;
attribute vec2 nor;

varying vec2 texCoord;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

void kore() {
    gl_Position =  P * V * M * vec4(pos, 1.0);
    
    texCoord = tex;
}
