#version 450

in vec3 pos;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void kore() {
	vec4 newPos = M * vec4(pos, 1.0);
	gl_Position = P * V * newPos;
}
