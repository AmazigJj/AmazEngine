#version 450
layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (set = 0, binding = 0) uniform sampler2D hdrBuffer;

layout ( push_constant ) uniform PushConstants {
	float exposure;
} consts;

vec3 toneMap(vec3 inColor, float exposure);
vec3 gammaCorrect(vec3 inColor, float gamma);

void main() {
	float gamma = 2.2;
	vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
	vec3 outColor;

	// reinhard tone mapping
	// mapped = hdrColor / (hdrColor + vec3(1.0));

	//Tone map
	outColor = toneMap(hdrColor, consts.exposure);

	// Gamma Correct
	outColor = gammaCorrect(outColor, gamma);

	FragColor = vec4(outColor, 1.0);
}

// exposure tone mapping
vec3 toneMap(vec3 inColor, float exposure) {
	return vec3(1.0) - exp(-inColor * exposure);
}

vec3 gammaCorrect(vec3 inColor, float gamma) {
	return pow(inColor, vec3(1.0 / gamma));
}