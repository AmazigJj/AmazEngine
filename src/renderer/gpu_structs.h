#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct GPUMaterial {
	VkDescriptorSet diffSet{ VK_NULL_HANDLE };
	VkDescriptorSet specSet{ VK_NULL_HANDLE };
};

struct GPUCameraData {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::mat4 viewproj;
};

struct GPUSceneData {
	alignas(16) glm::vec4 fogColor; // w is for exponent
	alignas(16) glm::vec4 fogDistances; //x for min, y for max, zw unused.
	alignas(16) glm::vec3 ambientColor;
	alignas(16) glm::vec3 sunlightDirection; //w for sun power
	alignas(16) glm::vec3 sunlightColor;
	alignas(16) glm::vec3 camPos;
	alignas(16) glm::mat4 lightSpaceMatrix;
	alignas(4) int shadowSamples;
	alignas(4) float shadowStride;
	alignas(4) float farPlane;
	alignas(4) float nearPlane;
};

struct GPUObjectData {
	alignas(16) glm::mat4 modelMatrix;
};

struct GPUShadowMapData {
	alignas(16) float shadowMapX;
	alignas(4) float shadowMapY;
	alignas(4) float shadowMapSize;
};

struct GPUPointLight {
	alignas(16) glm::vec3 lightPos;
	alignas(16) glm::mat4 lightSpaceMatrix[6];

	alignas(16) glm::vec3 lightColor;
	alignas(16) glm::vec3 ambientColor;

	alignas(4) float radius;
	alignas(4) float farPlane;

	alignas(16) GPUShadowMapData shadowMapData[6];
};

struct GPUDirLight {
	alignas(16) glm::vec3 lightPos;
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::mat4 lightSpaceMatrix;

	alignas(16) glm::vec3 lightColor;
	alignas(16) glm::vec3 ambientColor;

	alignas(16) GPUShadowMapData shadowMapData;
};

struct GPUSpotLight {
	alignas(16) glm::vec3 lightPos;
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::mat4 lightSpaceMatrix;

	alignas(16) glm::vec3 lightColor;
	alignas(16) glm::vec3 ambientColor;

	alignas(4) float radius;
	alignas(4) float constant;
	alignas(4) float linear;
	alignas(4) float quadratic;

	alignas(16) GPUShadowMapData shadowMapData;
};

struct GPUShadowPushConstants {
	int lightType;
	int lightIndex; // Note: for point lights this is (light * 6) + side
};

struct GPULightCullPushConstants {
	alignas(64) glm::mat4 viewMatrix;
	alignas(4) float frustum[4];
	alignas(4) float P00;
	alignas(4) float P11;
	alignas(4) float zNear;
	alignas(4) float zFar;
};

struct GPUDepthReducePushConstants {
	alignas(4) uint32_t index;
	alignas(8) glm::vec2 imageSize;
};

struct GPUCluster {
	alignas(16) glm::vec3 minPoint;
	alignas(16) glm::vec3 maxPoint;
	alignas(4) uint32_t index;
	alignas(4) uint32_t count;
};

struct GPUClusterPushConstant {
	alignas(4)	bool findClusters;
	alignas(4)	float zNear;
	alignas(4)	float zFar;
	alignas(64) glm::mat4 viewMatrix;
	alignas(64)	glm::mat4 inverseMatrix; // Needed if findClusters == true
};