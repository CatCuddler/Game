uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform vec3 lightPos;

attribute vec3 pos;
attribute vec2 tex;
attribute vec3 nor;
varying vec2 texCoord;
varying vec3 normal;
varying vec3 lightDirection;
varying vec3 eyeCoord;

void kore() {
	eyeCoord = (modelViewMatrix * vec4(pos, 1.0)).xyz;
	vec3 transformedLightPos = (modelViewMatrix * vec4(lightPos, 1.0)).xyz;
	lightDirection = transformedLightPos - eyeCoord;

	normal = (normalMatrix * vec4(nor, 0.0)).xyz;

	//perspective transform of eye coordinates
	float zMin = 1.0;
	float zMax = 100.0;
	if (eyeCoord.z < zMin) // to avoid some artifacts when moving through the model
		eyeCoord.z = -10000.0;
	gl_Position = vec4((zMin / eyeCoord.z) * eyeCoord.x, (zMin / eyeCoord.z) * eyeCoord.y, (eyeCoord.z - zMin)/zMax*2.0 - 1.0, 1.0);

	texCoord = tex;
}
