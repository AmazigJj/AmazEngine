#pragma once

#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <VkBootstrap.h>
#include <array>
#include <vector>
#include <span>
#include <fstream>
#include <glm/gtx/transform.hpp>
#include <deque>
#include <functional>
#include <algorithm>
#include <memory>
#include <imgui.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_vulkan.h>
#include <stb_image.h>
#include <optional>
#include <vulkan/vulkan_core.h>
#include <unordered_map>

#include "vk_mesh.h"
#include "vk_types.h"
#include "vk_initializers.h"
#include "gpu_structs.h"
#include "../input/Input.h"
#include "util/ShaderStages.h"
#include "backend/VkBackendRenderer.h"


constexpr unsigned int FRAME_OVERLAP = 2;

struct Material {
	VkDescriptorSet textureSet{ VK_NULL_HANDLE };
	VkDescriptorSet specularSet{ VK_NULL_HANDLE };
	VkPipeline pipeline{};
	VkPipelineLayout pipelineLayout{};
};

struct RenderObject {
	Mesh* mesh;

	Material* material;

	glm::mat4 transformMatrix;
};

struct DirLightObject {
	glm::vec3 lightPos;
	glm::vec3 lightDir;

	glm::vec3 lightColor;
	glm::vec3 ambientColor;
};

struct PointLightObject {
	glm::vec3 lightPos;

	glm::vec3 lightColor;
	glm::vec3 ambientColor;

	float radius;
};

struct SpotLightObject {
	glm::vec3 lightPos;
	glm::vec3 lightDir;
	float radius;

	glm::vec3 lightColor;
	glm::vec3 ambientColor;

	float constant;
	float linear;
	float quadratic;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call the function
		}

		deletors.clear();
	}
};

struct FrameData {
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkDescriptorSet globalDescriptor;

	AllocatedBuffer dirLightBuffer;
	AllocatedBuffer pointLightBuffer;
	AllocatedBuffer spotLightBuffer;
	AllocatedBuffer activeLightBuffer;
	AllocatedBuffer clustersBuffer;
	AllocatedBuffer lightIndicesBuffer;


	AllocatedBuffer objectBuffer;
	VkDescriptorSet objectDescriptor;

	AllocatedBuffer indirectBuffer;
	AllocatedBuffer indirectCount;
};

struct UploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

struct Texture {
	AllocatedImage image;
	VkImageView imageView;
};

struct ShaderStageInfo {
	amaz::eng::ShaderStages stage;
	VkShaderModule module;
};

// class PipelineBuilder {
// public:

// 	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
// 	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
// 	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
// 	VkViewport _viewport;
// 	VkRect2D _scissor;
// 	VkPipelineRasterizationStateCreateInfo _rasterizer;
// 	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
// 	VkPipelineMultisampleStateCreateInfo _multisampling;
// 	VkPipelineLayout _pipelineLayout;

// 	VkPipelineDepthStencilStateCreateInfo _depthStencil;

// 	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass, bool colorBlendingEnabled = true);
// 	PipelineBuilder& addShader(ShaderStageInfo shader);
// };

struct IndirectBatch {
	Mesh* mesh;
	Material* material;
	uint32_t first;
	uint32_t count;
};

class Renderer {
public:
	Renderer(int width, int height);
	~Renderer();

	void createWindow(int width, int height, bool fullscreen, bool borderless);
	void initVulkan(int verMajor, int verMinor, std::string appName);
	void initSwapchain(bool vsync);
	void cleanupFrameBuffers();
	void recreateFrameBuffers(uint32_t width, uint32_t height);
	void cleanupRenderBuffers();
	void recreateRenderBuffers();
	void initFrameBuffers();
	void initCommands();
	void initRenderPasseses();
	void initMainRenderpass();
	void initPrePassRenderpass();
	void initShadowPass();
	void initTonemapRenderPass();
	void initSyncStructures();
	void initDescriptors();
	void initShadowDescriptor();
	void initPipelines();
	VkPipeline initComputePipeline(std::string filePath, VkPipelineLayout pipelineLayoutInfo);
	void initComputePipelines();
	void createPyramidSetLayout();
	void createDepthPyramidImage();
	void initImgui();

	VkRenderPass createRenderPass(bool hasColor, VkFormat colorFormat, VkAttachmentLoadOp colorLoadOp, VkAttachmentStoreOp colorStoreOp, VkImageLayout finalColorLayout,
		VkFormat depthFormat, VkAttachmentLoadOp depthLoadOp, VkAttachmentStoreOp depthStoreOp);

	void draw(glm::vec3 camDir, Input* input);
	void mapData(std::span<RenderObject> renderObjects, glm::mat4 camView, glm::mat4 camProj, GPUSceneData sceneParameters, std::span<PointLightObject> lights);
	GPUPointLight generatePointLight(PointLightObject light, uint32_t tile);
	glm::mat4 genCubeMapViewMatrix(uint8_t face, glm::vec3 lightPos);
	void drawObjects(VkCommandBuffer cmd, std::span<RenderObject> renderObjects, glm::vec3 camPos, glm::vec3 camDir, GPUSceneData sceneParameters, std::span<PointLightObject> lights);
	void drawImguiWindow(Input* input);
	void drawPrePass(VkCommandBuffer cmd, std::span<RenderObject> renderObjects, GPUSceneData sceneParameters, uint32_t frameIndex);
	void genDepthPyramid(VkCommandBuffer cmd, uint32_t frameNumber);
	void cullLightsPass(VkCommandBuffer cmd, std::span<PointLightObject> lights, glm::mat4 viewMatrix, glm::mat4 projMatrix, uint32_t swapchainIndex, float zNear, float zFar);
	void clusterLightsPass(VkCommandBuffer cmd, bool findCluster, glm::mat4 viewMatrix, glm::mat4 inverseMatrix, float zNear, float zFar);
	void drawShadowPass(VkCommandBuffer cmd, std::span<RenderObject> renderObjects, GPUSceneData sceneParameters, std::span<PointLightObject> lights);
	void drawShadow(VkCommandBuffer cmd, std::span<IndirectBatch> draws, int type, int index, float x, float y, float size);
	void drawTonemapping(VkCommandBuffer cmd, uint32_t imageIndex);

	void sortObjects(std::span<RenderObject> renderObjects);
	std::vector<IndirectBatch> compactDraws(std::span<RenderObject> objects);
	void bindMaterial(const Material& material, VkCommandBuffer cmd, uint32_t frameOffset);
	void bindMesh(const Mesh& mesh, VkCommandBuffer cmd);
	template <typename T>
	void createStageAndCopyBuffer(std::span<T> data, AllocatedBuffer& bufferLocation, VkBufferUsageFlags usageFlags);
	FrameData& getCurrentFrame();

	size_t padUniformBufferSize(size_t originalSize);
	AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	bool loadShaderModule(std::string filePath, VkShaderModule& outShaderModule);
	Material& createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	void registerMaterial(std::string matTemplate, std::string name, std::optional<std::string> diffuseMap = std::nullopt, std::optional<std::string> specularMap = std::nullopt);
	std::tuple< VkPipeline, VkPipelineLayout> createPipeline(std::span<VkDescriptorSetLayout> setLayouts, std::span<VkPushConstantRange> pushConstants,
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages, VertexInputDescription vertexDescription);
	void loadImage(std::string filename, std::string textureName);
	bool loadImageFromFile(std::string file, AllocatedImage& outImage);
	void loadMesh(std::string name, std::string filename);
	void loadLight(glm::vec3 pos, glm::vec3 color, float radius);
	std::vector<Vertex> createCuboid(float x, float y, float z);
	std::vector<Vertex> createCube();
	std::vector<Vertex> createSquare(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight, glm::vec3 normal, glm::vec3 color = { 1.f, 1.f, 1.f });
	void uploadMesh(Mesh& mesh);
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
	void registerRenderObject(std::string mesh, std::string material, glm::mat4 transform);
	void registerRenderObject(std::string mesh, std::string material, glm::vec3 position);
	void registerRenderObject(std::string mesh, std::string material, glm::vec3 position, float scale, glm::vec3 rotation);
	void allocateTexture(VkImageView imageView, VkDescriptorPool descPool, VkDescriptorSetLayout* setLayout, VkDescriptorSet* textureSet);
	Material* getMaterial(const std::string& name);
	Mesh* getMesh(const std::string& name);
	void setPlayerPos(glm::vec3 pos);
	void setThirdPerson(bool toggle);
	void setPlayerPosAndDir(glm::vec3 pos, glm::vec3 dir);

	void createGPUImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, bool cubemap, AllocatedImage& image, VkImageView& imageView, bool destroy = true);
	VkFramebuffer createFrameBuffer(uint32_t width, uint32_t height, std::span<VkImageView> attachments, VkRenderPass renderPass, bool destroy = true);
	VkFramebuffer createShadowFrameBuffer(uint32_t width, uint32_t height, AllocatedImage& shadowDepthImage, VkImageView& shadowDepthImageView, bool cubemap);


	/*
	* Calculate a model -> world space transform matrix for a given position, scale and rotation
	*
	* @param position Position of the object within world space
	* @param scale Scale at which to transform the matrix
	* @rotation Rotation on each axis in degrees
	*
	* @return Transform matrix transforming from model to world space
	*/
	glm::mat4 calcTransformMatrix(glm::vec3 position, float scale, glm::vec3 rotation);


	VmaAllocator _allocator;

	DeletionQueue _mainDeletionQueue;

	SDL_Window* getWindow() {
		return _window.get();
	}

	float frametime;

private:
	struct {
		uint32_t width, height;
	} _winSize;

	struct {
		uint32_t width, height;
	} _actualWinSize;

	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window;
	std::shared_ptr<amaz::eng::VkBackendRenderer> backendRenderer;
	

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;

	VkSurfaceKHR _surface;
	VkPhysicalDevice _physicalDevice;
	VkDevice _device;
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VkPhysicalDeviceProperties _gpuProperties;


	VkSwapchainKHR _swapchain;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkFormat _swapchainImageFormat;

	AllocatedImage _depthImage;
	VkImageView _depthImageView;
	VkFormat _depthFormat;

	AllocatedImage _depthPyramid;
	std::vector<VkImageView> _depthPyramidViews;
	VkImageView _depthPyramidView;
	VkDescriptorSetLayout _depthPyramidSetLayout;
	VkPipeline _depthPyramidPipeline;
	VkPipelineLayout _depthPyramidPipelineLayout;
	VkSampler _depthSampler;
	std::vector<VkDescriptorSet> _depthPyramidSets;

	std::array<FrameData, FRAME_OVERLAP> _frames;
	std::vector<VkFramebuffer> _framebuffers;

	VkRenderPass _prePassRenderPass;

	VkRenderPass _mainRenderPass;
	std::vector<VkFramebuffer> _mainFramebuffers;
	VkFormat _mainFrameFormat;

	std::vector<AllocatedImage> _mainFrameImages;
	std::vector<VkImageView> _mainFrameImageViews;

	std::vector<AllocatedImage> _mainFrameDepthImages;
	std::vector<VkImageView> _mainFrameDepthImageViews;

	std::vector<VkDescriptorSet> _mainFrameImagesSets;


	UploadContext _uploadContext;

	VkRenderPass _renderPass;

	VkRenderPass _tonemapRenderPass;
	std::vector<VkFramebuffer> _tonemapFramebuffers;



	VkRenderPass _shadowRenderPass;
	VkPipeline _shadowPipeline;
	VkPipelineLayout _shadowPipelineLayout;

	VkPipeline _tonemapPipeline;
	VkPipelineLayout _tonemapPipelineLayout;

	VkPipeline _prePassPipeline;
	VkPipelineLayout _prePassPipelineLayout;
	std::vector<VkFramebuffer> _prePassFramebuffers;

	VkPipeline _lightCullPipeline;
	VkPipelineLayout _lightCullPipelineLayout;

	VkPipeline _clusterPipeline;
	VkPipelineLayout _clusterPipelineLayout;

	VkDescriptorSetLayout _shadowPassSetLayout;
	std::vector<VkDescriptorSet> _shadowPassDescriptorSets;


	VkFramebuffer _shadowAtlasFrameBuffer;
	AllocatedImage _shadowAtlasDepthImage;
	VkImageView _shadowAtlasDepthImageView;

	AllocatedImage _shadowAtlasImage;
	VkImageView _shadowAtlasImageView;

	VkFramebuffer _pointLightFrameBuffer;
	AllocatedImage _pointLightShadowImage;
	VkImageView _pointLightShadowImageView;

	AllocatedBuffer _sceneParameterBuffer;

	AllocatedBuffer _lightBuffer;

	VkDescriptorSetLayout _globalSetLayout;
	VkDescriptorSetLayout _objectSetLayout;
	VkDescriptorPool _descriptorPool;

	VkDescriptorSetLayout _singleTextureSetLayout;
	VkDescriptorSetLayout _specularMapSetLayout;

	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Mesh> _meshes;
	std::unordered_map<std::string, Texture> _loadedTextures;

	std::vector<RenderObject> _renderables;
	std::vector<DirLightObject> _dirLights;
	std::vector<PointLightObject> _pointLights;
	std::vector<SpotLightObject> _spotLights;

	RenderObject* _player_renderable;

	uint32_t _frameNumber{ 0 };

	float fov = 70.f;

	glm::vec3 renderPos;

	bool thirdPerson = false;

};