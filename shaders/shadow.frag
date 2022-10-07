#version 460
layout (location = 0) out vec4 outFragColor;

layout (location = 0) flat in int lightType;
layout (location = 1) in vec3 fragPos;
layout (location = 2) flat in vec3 lightPos;
layout (location = 3) flat in float farPlane;

void main()
{
	if (lightType == 1) {
		vec3 lightVec = fragPos - lightPos;
		float lightDistance = length(lightVec);
		gl_FragDepth = (lightDistance / farPlane);
	} else {
		gl_FragDepth = gl_FragCoord.z;
	}
}