#version 450

#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 vColor;

out vec4 FragColor;

void kore() {
	FragColor = vColor;
}
