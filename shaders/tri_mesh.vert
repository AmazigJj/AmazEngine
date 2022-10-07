#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out vec3 Normal;
layout (location = 3) out vec3 FragPos;
layout (location = 4) out vec4 viewPos;
layout (location = 5) out vec4 viewprojPos;

layout(set = 0, binding = 0) uniform CameraBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;

layout(set = 0, binding = 1) uniform SceneData{
    vec4 fogColor; // w is for exponent
	vec4 fogDistances; //x for min, y for max, zw unused.
	vec3 ambientColor;
	vec3 sunlightDirection; //w for sun power
	vec3 sunlightColor;
	vec3 camPos;
	mat4 lightSpaceMatrix;
	int shadowSamples;
	float shadowStride;
	float farPlane;
	float nearPlane;
} sceneData;

struct ObjectData{
	mat4 model;
};

//all object matrices
layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

//push constants block
layout( push_constant ) uniform constants {
	vec4 data;
	mat4 render_matrix;
} PushConstants;


void main() {
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
	outColor = vColor;
	texCoord = vTexCoord;
	Normal = mat3(transpose(inverse(modelMatrix))) * vNormal;
	FragPos = (modelMatrix * vec4(vPosition, 1.0)).xyz;
	viewPos = cameraData.view * vec4(FragPos, 1);
	viewprojPos = cameraData.proj * viewPos;

	gl_Position = viewprojPos;
}
