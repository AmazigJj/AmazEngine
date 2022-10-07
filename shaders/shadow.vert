#version 460
layout (location = 0) in vec3 vPosition;

layout (location = 0) out int lightType;
layout (location = 1) out vec3 fragPos;
layout (location = 2) out vec3 lightPos;
layout (location = 3) out float farPlane;


struct ShadowMapData {
	float shadowMapX;		// 1 = 1 tile, 1024 pixels
	float shadowMapY;		// 1 = 1 tile, 1024 pixels
	float shadowMapSize;	// 1 = 1024 x 1024 pixels
};

struct PointLight {
	vec3 lightPos;
	mat4 lightSpaceMatrix[6];

	vec3 lightColor;
	vec3 ambientColor;

	float radius;
	float farPlane;

	ShadowMapData shadowMapData[6];
};

struct DirLight {
	vec3 lightPos;
	vec3 lightDir;
	mat4 lightSpaceMatrix;
	
	vec3 lightColor;
	vec3 ambientColor;

	ShadowMapData shadowMapData;
};

struct SpotLight {
	vec3 lightPos;
	vec3 lightDir;
	mat4 lightSpaceMatrix;

	vec3 lightColor;
	vec3 ambientColor;

	float radius;
	float constant;
    float linear;
    float quadratic;

	ShadowMapData shadowMapData;
};

layout ( push_constant ) uniform PushConstants {
	int lightType;
	int lightIndex; // Note: for point lights this is (light * 6) + side
} consts;

struct ObjectData{
	mat4 model;
};

//all object matrices
layout(std140,set = 0, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

//all directional lights
layout(std140,set = 0, binding = 1) readonly buffer DirLightBuffer {
	int count;
	DirLight lights[];
} dirLightBuffer;

//all point lights
layout(std140,set = 0, binding = 2) readonly buffer PointLightBuffer {
	int count;
	PointLight lights[];
} pointLightBuffer;

//all spot lights
layout(std140,set = 0, binding = 3) readonly buffer SpotLightBuffer {
	int count;
	SpotLight lights[];
} spotLightBuffer;

void main()
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;

	mat4 lightSpaceMatrix;

	ShadowMapData shadowMapData;

	lightType = consts.lightType;
	if (lightType == 0) {			// Dir Light
		lightSpaceMatrix = dirLightBuffer.lights[consts.lightIndex].lightSpaceMatrix;
		lightPos = dirLightBuffer.lights[consts.lightIndex].lightPos;
		shadowMapData = dirLightBuffer.lights[consts.lightIndex].shadowMapData;
	} else if (lightType == 1) {	// Point Light
		lightSpaceMatrix = pointLightBuffer.lights[consts.lightIndex/6].lightSpaceMatrix[consts.lightIndex%6];
		lightPos = pointLightBuffer.lights[consts.lightIndex/6].lightPos;
		shadowMapData = pointLightBuffer.lights[consts.lightIndex/6].shadowMapData[consts.lightIndex%6];
		farPlane = pointLightBuffer.lights[consts.lightIndex/6].radius;
	} else {						// Spotight
		lightSpaceMatrix = spotLightBuffer.lights[consts.lightIndex].lightSpaceMatrix;
		lightPos = spotLightBuffer.lights[consts.lightIndex].lightPos;
		shadowMapData = spotLightBuffer.lights[consts.lightIndex].shadowMapData;
	}


	fragPos = (modelMatrix * vec4(vPosition, 1.0)).xyz;
	gl_Position = lightSpaceMatrix * vec4(fragPos, 1.0);

	// if (consts.lightType == 1) {
	// 	vec3 lightVec = fragPos - lightPos;
	// 	float lightDistance = length(lightVec);
	// 	gl_Position.z = (lightDistance / 25.f);
	// }
	

}