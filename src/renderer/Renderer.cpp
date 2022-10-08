#include "SDL_video.h"
#include "glm/exponential.hpp"
#include "imgui.h"
#include "vk_mem_alloc.h"
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "Renderer.h"
#include <exception>
#include "vk_descriptor.h"
#include <sstream>
#include "PipelineBuilder.h"

bool vsync = false;

void mat4Print(glm::mat4 matrix);

Renderer::Renderer(int width, int height) : _window(nullptr, SDL_DestroyWindow) {

	_winSize.width = width;
	_winSize.height = height;

	_actualWinSize.width = width;
	_actualWinSize.height = height;

	createWindow(width, height, false, false);
	initVulkan(1, 2, "TestApp");
	initSwapchain(false);
	initRenderPasseses();
	initCommands();
	initSyncStructures();
	initFrameBuffers();
	initDescriptors();
	initPipelines();
	initImgui();

}

Renderer::~Renderer() {

	vkWaitForFences(_device, 1, &_frames[0]._renderFence, true, 1000000000);
	vkWaitForFences(_device, 1, &_frames[1]._renderFence, true, 1000000000);

	_mainDeletionQueue.flush();

	cleanupRenderBuffers();

	cleanupFrameBuffers();

	vmaDestroyAllocator(_allocator);

	vkDestroyDevice(_device, nullptr);
	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
	vkDestroyInstance(_instance, nullptr);
}

void Renderer::createWindow(int width, int height, bool fullscreen, bool borderless) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "SDL INIT FAILED!" << std::endl;
	}

	int windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

	if (fullscreen) windowFlags = windowFlags | SDL_WINDOW_FULLSCREEN;
	if (borderless) windowFlags = windowFlags | SDL_WINDOW_BORDERLESS;

	_window.reset(SDL_CreateWindow(
		"Amaz Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		windowFlags
	));

	if (!_window) {

		const char* e = SDL_GetError();
		std::cout << e << std::endl;
	}


}

void Renderer::initVulkan(int verMajor, int verMinor, std::string appName) {

	vkb::InstanceBuilder builder;

	//make the Vulkan instance, with basic debug features
	auto vkb_inst = builder.set_app_name(appName.c_str())
		.set_engine_name("AmazEngine")
		.request_validation_layers(true)
		.require_api_version(verMajor, verMinor, 0)
		.use_default_debug_messenger()
		.build()
		.value();


	_instance = vkb_inst.instance;
	_debugMessenger = vkb_inst.debug_messenger;


	// get the surface of the window we opened with SDL
	if (SDL_Vulkan_CreateSurface(_window.get(), vkb_inst.instance, &_surface) != SDL_TRUE) {
		std::cout << "Creating Surface Failed!" << std::endl;
		const char* e = SDL_GetError();
		std::cout << e << std::endl;
	}

	VkPhysicalDeviceVulkan12Features features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = nullptr,
		.drawIndirectCount = VK_TRUE
	};

	//use vkbootstrap to select a GPU.
	//We want a GPU that can write to the SDL surface and supports Vulkan Version Specific
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(verMajor, verMinor)
		.set_surface(_surface)
		.set_required_features_12(features)
		.select()
		.value();


	//create the final Vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
		.pNext = nullptr,
		.shaderDrawParameters = VK_TRUE
	};
	vkb::Device vkbDevice = deviceBuilder.add_pNext(&shader_draw_parameters_features).build().value();

	_physicalDevice = vkbDevice.physical_device;
	_device = vkbDevice.device;
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo = {
		.physicalDevice = vkbDevice.physical_device,
		.device = vkbDevice.device,
		.instance = vkb_inst.instance
	};
	vmaCreateAllocator(&allocatorInfo, &_allocator);

	vkGetPhysicalDeviceProperties(vkbDevice.physical_device, &_gpuProperties);

	std::cout << "The GPU has a minimum buffer alignment of " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
	std::cout << "The GPU has a maximum sampler allocation of " << _gpuProperties.limits.maxSamplerAllocationCount << std::endl;
	std::cout << "The GPU has a maxPerStageDescriptorSampledImages of " << _gpuProperties.limits.maxPerStageDescriptorSampledImages << std::endl;

	uint32_t apiVersion = _gpuProperties.apiVersion;
	uint32_t apiMajor = (apiVersion & (1 << 29) - 1) >> 22;
	uint32_t apiMinor = (apiVersion & (1 << 22) - 1) >> 12;
	uint32_t apiPatch = apiVersion & (1 << 12) - 1;


	std::cout << "Vulkan version is: " << apiMajor << "." << apiMinor << "." << apiPatch << "\n";

	std::cout << "Vulkan version is: " << apiVersion << "\n";

	

	backendRenderer = std::make_shared<amaz::eng::VkBackendRenderer>(_device);

}

void Renderer::initSwapchain(bool vsync) {
	vkb::SwapchainBuilder swapchainBuilder{ _physicalDevice,_device,_surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
		.set_desired_extent(_actualWinSize.width, _actualWinSize.height)
		.build()
		.value();

	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	std::cout << "Swapchain images: " << _swapchainImages.size() << "\n";

	_swapchainImageFormat = vkbSwapchain.image_format;

	//hardcoding the depth format to 32 bit float
	_depthFormat = VK_FORMAT_D32_SFLOAT;
}

void Renderer::cleanupFrameBuffers() {

	for (int i = 0; i < _tonemapFramebuffers.size(); i++)
		vkDestroyFramebuffer(_device, _tonemapFramebuffers[i], nullptr);
	for (int i = 0; i < _swapchainImageViews.size(); i++)
		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);
}

void Renderer::recreateFrameBuffers(uint32_t width, uint32_t height) {
	vkDeviceWaitIdle(_device);

	cleanupFrameBuffers();

	_actualWinSize = {width, height};
	initSwapchain(vsync);

	for (int i = 0; i < _tonemapFramebuffers.size(); i++) {
		std::array<VkImageView, 1> imageAttachments = {_swapchainImageViews[i]};
		_tonemapFramebuffers[i] = createFrameBuffer(width, height, imageAttachments, _tonemapRenderPass, false);
	}
}

void Renderer::initRenderPasseses() {
	_mainFrameFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_depthFormat = VK_FORMAT_D32_SFLOAT;

	initMainRenderpass();
	initPrePassRenderpass();
	initShadowPass();
	initTonemapRenderPass();
}

void Renderer::initMainRenderpass() {
	// ready to be read for post-processing etc.
	auto colorAttachment = vkinit::attachmentDescription(_mainFrameFormat, VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	auto depthAttachment = vkinit::attachmentDescription(_depthFormat, VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	std::vector<VkAttachmentReference> colourAttachments = {colorAttachmentRef};

	auto subpass = vkinit::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachments, depthAttachmentRef);

	auto dependency = vkinit::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	
	auto depthDependency = vkinit::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

	std::vector<VkAttachmentDescription> attachments = { colorAttachment,depthAttachment };
	std::vector<VkSubpassDescription> subpasses = { subpass };
	std::vector<VkSubpassDependency> dependencies = { dependency, depthDependency };


	auto renderPassInfo = vkinit::renderPassCreateInfo(attachments, subpasses, dependencies);

	vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_mainRenderPass);

	_mainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(_device, _mainRenderPass, nullptr);
		});
}

void Renderer::initPrePassRenderpass() {

	auto depthAttachment = vkinit::attachmentDescription(_depthFormat, VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	VkAttachmentReference depthAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	std::vector<VkAttachmentReference> colourAttachments = {};

	auto subpass = vkinit::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachments, depthAttachmentRef);

	std::vector<VkAttachmentDescription> attachments = { depthAttachment };
	std::vector<VkSubpassDescription> subpasses = { subpass };
	std::vector<VkSubpassDependency> dependencies = { };


	auto renderPassInfo = vkinit::renderPassCreateInfo(attachments, subpasses, dependencies);

	vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_prePassRenderPass);

	_mainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(_device, _prePassRenderPass, nullptr);
		});
}

void Renderer::initShadowPass() {

	auto depthAttachment = vkinit::attachmentDescription(_depthFormat, VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

	VkAttachmentReference depthAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = vkinit::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, {}, depthAttachmentRef);

	std::vector<VkAttachmentDescription> attachments = { depthAttachment };
	std::vector<VkSubpassDescription> subpasses = { subpass };

	auto renderPassInfo = vkinit::renderPassCreateInfo(attachments, subpasses, {});

	vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_shadowRenderPass);

	_mainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(_device, _shadowRenderPass, nullptr);
		});
}

void Renderer::initTonemapRenderPass() {

	// ready to be presented
	auto colorAttachment = vkinit::attachmentDescription(_swapchainImageFormat, VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depthAttachmentRef = {
		.attachment = VK_ATTACHMENT_UNUSED,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	std::vector<VkAttachmentReference> colourAttachments = {colorAttachmentRef};

	auto subpass = vkinit::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachments, depthAttachmentRef);

	auto dependency = vkinit::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	std::vector<VkAttachmentDescription> attachments = { colorAttachment };
	std::vector<VkSubpassDescription> subpasses = { subpass };
	std::vector<VkSubpassDependency> dependencies = { dependency };


	auto renderPassInfo = vkinit::renderPassCreateInfo(attachments, subpasses, dependencies);

	vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_tonemapRenderPass);

	_mainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(_device, _tonemapRenderPass, nullptr);
		});
}

VkRenderPass Renderer::createRenderPass(bool hasColor, VkFormat colorFormat, VkAttachmentLoadOp colorLoadOp, VkAttachmentStoreOp colorStoreOp, VkImageLayout finalColorLayout,
	VkFormat depthFormat, VkAttachmentLoadOp depthLoadOp, VkAttachmentStoreOp depthStoreOp) {
	std::vector<VkAttachmentReference> colourAttachments = {};
	
	std::vector<VkAttachmentDescription> attachments = {};
	std::vector<VkSubpassDescription> subpasses = {};
	std::vector<VkSubpassDependency> dependencies = {};

	if (hasColor) {
		auto colorAttachment = vkinit::attachmentDescription(colorFormat, VK_SAMPLE_COUNT_1_BIT,
			colorLoadOp, colorStoreOp,
			VK_IMAGE_LAYOUT_UNDEFINED, finalColorLayout);

		VkAttachmentReference colorAttachmentRef = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		auto colorDependency = vkinit::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

		colourAttachments.push_back(colorAttachmentRef);
		attachments.push_back(colorAttachment);
		dependencies.push_back(colorDependency);
		
	}

	auto depthAttachment = vkinit::attachmentDescription(depthFormat, VK_SAMPLE_COUNT_1_BIT,
		depthLoadOp, depthStoreOp,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	auto subpass = vkinit::subpassDescription(VK_PIPELINE_BIND_POINT_GRAPHICS, colourAttachments, depthAttachmentRef);

	auto depthDependency = vkinit::subpassDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

	auto renderPassInfo = vkinit::renderPassCreateInfo(attachments, subpasses, dependencies);

	VkRenderPass renderPass;
	vkCreateRenderPass(_device, &renderPassInfo, nullptr, &renderPass);

	_mainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(_device, renderPass, nullptr);
		});

	return renderPass;
}

void Renderer::initFrameBuffers() {
	//create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering

	VkFramebufferCreateInfo fb_info = vkinit::frameBufferCreateInfo(_renderPass, _winSize.width, _winSize.height);

	//grab how many images we have in the swapchain
	const uint32_t swapchain_imagecount = _swapchainImages.size();
	_framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	_mainFramebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);
	_tonemapFramebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);
	_prePassFramebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	_mainFrameImages = std::vector<AllocatedImage>(swapchain_imagecount);
	_mainFrameImageViews = std::vector<VkImageView>(swapchain_imagecount);

	_mainFrameDepthImages = std::vector<AllocatedImage>(swapchain_imagecount);
	_mainFrameDepthImageViews = std::vector<VkImageView>(swapchain_imagecount);

	//create framebuffers for each of the swapchain image views
	for (int i = 0; i < swapchain_imagecount; i++) {

		createGPUImage(_winSize.width, _winSize.height, _mainFrameFormat,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			false, _mainFrameImages[i], _mainFrameImageViews[i], false);

		createGPUImage(_winSize.width, _winSize.height, _depthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
			false, _mainFrameDepthImages[i], _mainFrameDepthImageViews[i], false);

		{
			std::array<VkImageView, 2> attachments = {_mainFrameImageViews[i], _mainFrameDepthImageViews[i]};
			_mainFramebuffers[i] = createFrameBuffer(_winSize.width, _winSize.height, attachments, _mainRenderPass, false);
		}
		{
			std::array<VkImageView, 1> attachments = {_mainFrameDepthImageViews[i]};
			_prePassFramebuffers[i] = createFrameBuffer(_winSize.width, _winSize.height, attachments, _prePassRenderPass, false);
		}

		{
			std::array<VkImageView, 1> attachments = {_swapchainImageViews[i]};
			_tonemapFramebuffers[i] = createFrameBuffer(_winSize.width, _winSize.height, attachments, _tonemapRenderPass, false);
		}
	}

	_shadowAtlasFrameBuffer = createShadowFrameBuffer(8192, 8192,
		_shadowAtlasImage,_shadowAtlasImageView, false);
}

void Renderer::cleanupRenderBuffers() {
	for (int i = 0; i < _mainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(_device, _mainFramebuffers[i], nullptr);
		vkDestroyFramebuffer(_device, _prePassFramebuffers[i], nullptr);

		vkDestroyImageView(_device, _mainFrameImageViews[i], nullptr);
		vmaDestroyImage(_allocator, _mainFrameImages[i]._image, _mainFrameImages[i]._allocation);

		vkDestroyImageView(_device, _mainFrameDepthImageViews[i], nullptr);
		vmaDestroyImage(_allocator, _mainFrameDepthImages[i]._image, _mainFrameDepthImages[i]._allocation);

	}
	
	_depthPyramidSets.clear();
}

void Renderer::recreateRenderBuffers() {
	vkDeviceWaitIdle(_device);

	cleanupRenderBuffers();

	for (int i = 0; i < _swapchainImages.size(); i++) {

		createGPUImage(_winSize.width, _winSize.height, _mainFrameFormat,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			false, _mainFrameImages[i], _mainFrameImageViews[i], false);

		createGPUImage(_winSize.width, _winSize.height, _depthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
			false, _mainFrameDepthImages[i], _mainFrameDepthImageViews[i], false);

		std::array<VkImageView, 2> attachments = {_mainFrameImageViews[i], _mainFrameDepthImageViews[i]};

		_mainFramebuffers[i] = createFrameBuffer(_winSize.width, _winSize.height, attachments, _mainRenderPass, false);

		{
			std::array<VkImageView, 1> attachments = {_mainFrameDepthImageViews[i]};
			_prePassFramebuffers[i] = createFrameBuffer(_winSize.width, _winSize.height, attachments, _prePassRenderPass, false);
		}

		VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);

		VkSampler sampler;
		vkCreateSampler(_device, &samplerInfo, nullptr, &sampler);

		VkDescriptorImageInfo imageBufferInfo{
			.sampler = sampler,
			.imageView = _mainFrameImageViews[i],
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		auto mainFrameDescriptorSetBuilder = amaz::eng::DescriptorBuilder::init()
			.add_image(0, 1, amaz::eng::BindingType::IMAGE_SAMPLER, amaz::eng::ShaderStages::FRAGMENT, imageBufferInfo);

		_mainFrameImagesSets[i] = mainFrameDescriptorSetBuilder.build_set(_device, _descriptorPool, _singleTextureSetLayout);
	}

}

void Renderer::createGPUImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, bool cubemap, AllocatedImage& image, VkImageView& imageView, bool destroy /* = true */) {
	VkExtent3D extent = {
		width,
		height,
		1
	};

	auto imageInfo = vkinit::image_create_info(format,
		usage, extent,
		cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0, cubemap ? 6 : 1);

	VmaAllocationCreateInfo imageAllocInfo{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};

	vmaCreateImage(_allocator, &imageInfo, &imageAllocInfo, &image._image, &image._allocation, nullptr);

	auto viewInfo = vkinit::imageview_create_info(format, image._image, 
		aspect, cubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D);

	vkCreateImageView(_device, &viewInfo, nullptr, &imageView);
	if (destroy) {
		_mainDeletionQueue.push_function([=]() {
			vkDestroyImageView(_device, imageView, nullptr);
			vmaDestroyImage(_allocator, image._image, image._allocation);
		});
	}
}

VkFramebuffer Renderer::createFrameBuffer(uint32_t width, uint32_t height, std::span<VkImageView> attachments, VkRenderPass renderPass, bool destroy /* = true */) {
	VkFramebufferCreateInfo fbInfo = vkinit::frameBufferCreateInfo(renderPass, width, height);

	fbInfo.pAttachments = attachments.data();
	fbInfo.attachmentCount = attachments.size();

	VkFramebuffer frameBuffer;

	vkCreateFramebuffer(_device, &fbInfo, nullptr, &frameBuffer);

	if (destroy) {
		_mainDeletionQueue.push_function([=]() {
			vkDestroyFramebuffer(_device, frameBuffer, nullptr);
		});
	}

	return frameBuffer;
}

VkFramebuffer Renderer::createShadowFrameBuffer(uint32_t width, uint32_t height, AllocatedImage& shadowDepthImage, VkImageView& shadowDepthImageView, bool cubemap) {

	VkExtent3D shadowImageExtent = {
		width,
		height,
		1
	};

	auto depthImageInfo = vkinit::image_create_info(_depthFormat,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, shadowImageExtent,
		cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0, cubemap ? 6 : 1);

	VmaAllocationCreateInfo depthImageAllocInfo{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};

	vmaCreateImage(_allocator, &depthImageInfo, &depthImageAllocInfo, &shadowDepthImage._image, &shadowDepthImage._allocation, nullptr);

	auto depthViewInfo = vkinit::imageview_create_info(_depthFormat, shadowDepthImage._image, 
		VK_IMAGE_ASPECT_DEPTH_BIT, cubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D);

	vkCreateImageView(_device, &depthViewInfo, nullptr, &shadowDepthImageView);

	immediateSubmit([&](VkCommandBuffer cmd) {
			VkImageSubresourceRange range{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			};

			VkImageMemoryBarrier imageBarrier_toDepthReadOnly {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

				.srcAccessMask = VK_ACCESS_NONE,
				.dstAccessMask = VK_ACCESS_NONE,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
				.image = shadowDepthImage._image,
				.subresourceRange = range,
			};

			//barrier the image into the layout
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toDepthReadOnly);
		});

	_mainDeletionQueue.push_function([=]() {
		vkDestroyImageView(_device, shadowDepthImageView, nullptr);
		vmaDestroyImage(_allocator, shadowDepthImage._image, shadowDepthImage._allocation);
	});

	std::array<VkImageView, 1> attachments = {shadowDepthImageView};
	
	VkFramebufferCreateInfo fbInfo = vkinit::frameBufferCreateInfo(_shadowRenderPass, width, height);

	fbInfo.pAttachments = attachments.data();
	fbInfo.attachmentCount = attachments.size();

	VkFramebuffer frameBuffer;

	vkCreateFramebuffer(_device, &fbInfo, nullptr, &frameBuffer);

	_mainDeletionQueue.push_function([=]() {
		vkDestroyFramebuffer(_device, frameBuffer, nullptr);
		});

	return frameBuffer;
}

void Renderer::initCommands() {
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (auto& frame : _frames) {
		vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &frame._commandPool);

		//allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frame._commandPool, 1);

		vkAllocateCommandBuffers(_device, &cmdAllocInfo, &frame._mainCommandBuffer);

		_mainDeletionQueue.push_function([=]() {
			vkDestroyCommandPool(_device, frame._commandPool, nullptr);
			});
	}

	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily);
	//create pool for upload context
	vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool);

	_mainDeletionQueue.push_function([=]() {
		vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);
		});

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);

	vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_uploadContext._commandBuffer);
}

void Renderer::initSyncStructures() {
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();

	vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence);
	_mainDeletionQueue.push_function([=]() {
		vkDestroyFence(_device, _uploadContext._uploadFence, nullptr);
		});


	for (auto& frame : _frames) {
		vkCreateFence(_device, &fenceCreateInfo, nullptr, &frame._renderFence);

		//enqueue the destruction of the fence
		_mainDeletionQueue.push_function([=]() {
			vkDestroyFence(_device, frame._renderFence, nullptr);
			});

		vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame._presentSemaphore);
		vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame._renderSemaphore);

		//enqueue the destruction of semaphores
		_mainDeletionQueue.push_function([=]() {
			vkDestroySemaphore(_device, frame._presentSemaphore, nullptr);
			vkDestroySemaphore(_device, frame._renderSemaphore, nullptr);
			});
	}


}

void Renderer::initDescriptors() {
	const size_t sceneParamBufferSize = (padUniformBufferSize(sizeof(GPUCameraData)) + padUniformBufferSize(sizeof(GPUSceneData))) * FRAME_OVERLAP;

	_sceneParameterBuffer = createBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//create a descriptor pool that will hold 10 uniform buffers and 10 dynamic uniform buffers
	std::vector<VkDescriptorPoolSize> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = 2000,
		.poolSizeCount = (uint32_t)sizes.size(),
		.pPoolSizes = sizes.data()
	};

	vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptorPool);


	auto globalDescriptorLayoutBuilder = amaz::eng::DescriptorBuilder::init()
		.add_buffer(0, 1, amaz::eng::BindingType::UNIFORM_BUFFER_DYNAMIC,
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE)
		.add_buffer(1, 1, amaz::eng::BindingType::UNIFORM_BUFFER_DYNAMIC,
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE)
		.add_image(2, 1, amaz::eng::BindingType::IMAGE_SAMPLER,
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE);
		
	_globalSetLayout = globalDescriptorLayoutBuilder.build_layout(_device);

	auto objectDescriptorLayoutBuilder = amaz::eng::DescriptorBuilder::init()
		.add_buffer(0, 1, amaz::eng::BindingType::STORAGE_BUFFER, 
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE)
		.add_buffer(1, 1, amaz::eng::BindingType::STORAGE_BUFFER,
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE)
		.add_buffer(2, 1, amaz::eng::BindingType::STORAGE_BUFFER, 
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE)
		.add_buffer(3, 1, amaz::eng::BindingType::STORAGE_BUFFER,
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE)
		.add_buffer(4, 1, amaz::eng::BindingType::STORAGE_BUFFER,
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE)
		.add_buffer(5, 1, amaz::eng::BindingType::STORAGE_BUFFER,
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE)
		.add_buffer(6, 1, amaz::eng::BindingType::STORAGE_BUFFER,
			amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE);
	
	_objectSetLayout = objectDescriptorLayoutBuilder.build_layout(_device);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		constexpr int MAX_OBJECTS = 1000000;
		_frames[i].objectBuffer = createBuffer(sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		
		constexpr int MAX_DIR_LIGHTS = 1000;
		_frames[i].dirLightBuffer = createBuffer(sizeof(int) + (sizeof(GPUDirLight) * MAX_DIR_LIGHTS), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		constexpr int MAX_POINT_LIGHTS = 1000;
		_frames[i].pointLightBuffer = createBuffer(sizeof(int) + (sizeof(GPUPointLight) * MAX_POINT_LIGHTS), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		constexpr int MAX_SPOT_LIGHTS = 1000;
		_frames[i].spotLightBuffer = createBuffer(sizeof(int) + (sizeof(GPUSpotLight) * MAX_SPOT_LIGHTS), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		constexpr int MAX_ACTIVE_LIGHTS = 1000;
		_frames[i].activeLightBuffer = createBuffer(sizeof(uint32_t) + (sizeof(uint32_t) * 2 * MAX_ACTIVE_LIGHTS), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		constexpr int CLUSTER_COUNT = 16 * 8 * 24;
		_frames[i].clustersBuffer = createBuffer(sizeof(GPUCluster) * CLUSTER_COUNT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		constexpr int MAX_LIGHT_INDICES = 1000;
		_frames[i].lightIndicesBuffer = createBuffer(sizeof(uint32_t) + (sizeof(uint32_t) * MAX_LIGHT_INDICES), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		const int MAX_DRAWS = 1000;
		_frames[i].indirectBuffer = createBuffer(MAX_DRAWS * sizeof(VkDrawIndexedIndirectCommand), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		_frames[i].indirectCount = createBuffer(sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VkSampler sampler;
		vkCreateSampler(_device, &samplerInfo, nullptr, &sampler);

		VkDescriptorImageInfo shadowBufferInfo{
			.sampler = sampler,
			.imageView = _shadowAtlasImageView,
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL 
		};

		auto globalDescriptorBuilder = amaz::eng::DescriptorBuilder::init()
			.add_buffer(0, 1, amaz::eng::BindingType::UNIFORM_BUFFER_DYNAMIC,
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_sceneParameterBuffer, 0, sizeof(GPUCameraData)))
			.add_buffer(1, 1, amaz::eng::BindingType::UNIFORM_BUFFER_DYNAMIC,
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_sceneParameterBuffer, 0, sizeof(GPUSceneData)))
			.add_image(2, 1, amaz::eng::BindingType::IMAGE_SAMPLER,
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				shadowBufferInfo);
		
		_frames[i].globalDescriptor = globalDescriptorBuilder.build_set(_device, _descriptorPool, _globalSetLayout);


		auto objectDescriptorSetBuilder = amaz::eng::DescriptorBuilder::init()
			.add_buffer(0, 1, amaz::eng::BindingType::STORAGE_BUFFER, 
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_frames[i].objectBuffer, 0, sizeof(GPUObjectData) * MAX_OBJECTS))
			.add_buffer(1, 1, amaz::eng::BindingType::STORAGE_BUFFER,
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_frames[i].dirLightBuffer, 0, sizeof(uint32_t) + sizeof(GPUDirLight) * MAX_DIR_LIGHTS))
			.add_buffer(2, 1, amaz::eng::BindingType::STORAGE_BUFFER, 
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_frames[i].pointLightBuffer, 0, sizeof(uint32_t) + sizeof(GPUPointLight) * MAX_POINT_LIGHTS))
			.add_buffer(3, 1, amaz::eng::BindingType::STORAGE_BUFFER,
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_frames[i].spotLightBuffer, 0, sizeof(uint32_t) + sizeof(GPUSpotLight) * MAX_SPOT_LIGHTS))
			.add_buffer(4, 1, amaz::eng::BindingType::STORAGE_BUFFER,
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_frames[i].activeLightBuffer, 0, sizeof(uint32_t) + sizeof(uint32_t) * 2 * MAX_ACTIVE_LIGHTS))
			.add_buffer(5, 1, amaz::eng::BindingType::STORAGE_BUFFER,
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_frames[i].clustersBuffer, 0, sizeof(GPUCluster) * CLUSTER_COUNT))
			.add_buffer(6, 1, amaz::eng::BindingType::STORAGE_BUFFER,
				amaz::eng::ShaderStages::VERTEX | amaz::eng::ShaderStages::FRAGMENT | amaz::eng::ShaderStages::COMPUTE,
				vkinit::descriptorBufferInfo(_frames[i].lightIndicesBuffer, 0, sizeof(uint32_t) + (sizeof(uint32_t) * MAX_LIGHT_INDICES)));

		_frames[i].objectDescriptor = objectDescriptorSetBuilder.build_set(_device, _descriptorPool, _objectSetLayout);
	}

	auto textureDescriptorLayoutBuilder = amaz::eng::DescriptorBuilder::init()
		.add_buffer(0, 1, amaz::eng::BindingType::IMAGE_SAMPLER, amaz::eng::ShaderStages::FRAGMENT);

	_singleTextureSetLayout = textureDescriptorLayoutBuilder.build_layout(_device);

	_specularMapSetLayout = textureDescriptorLayoutBuilder.build_layout(_device);

	// delete EVERYTHING
	_mainDeletionQueue.push_function([&]() {
		// add descriptor set layout to deletion queues
		vkDestroyDescriptorSetLayout(_device, _globalSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _objectSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _singleTextureSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _specularMapSetLayout, nullptr);

		// add buffers to deletion queues
		vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
		vmaDestroyBuffer(_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);
		for (int i = 0; i < FRAME_OVERLAP; i++) {
			vmaDestroyBuffer(_allocator, _frames[i].objectBuffer._buffer, _frames[i].objectBuffer._allocation);
			vmaDestroyBuffer(_allocator, _frames[i].dirLightBuffer._buffer, _frames[i].dirLightBuffer._allocation);
			vmaDestroyBuffer(_allocator, _frames[i].pointLightBuffer._buffer, _frames[i].pointLightBuffer._allocation);
			vmaDestroyBuffer(_allocator, _frames[i].spotLightBuffer._buffer, _frames[i].spotLightBuffer._allocation);
			vmaDestroyBuffer(_allocator, _frames[i].activeLightBuffer._buffer, _frames[i].activeLightBuffer._allocation);
			vmaDestroyBuffer(_allocator, _frames[i].indirectBuffer._buffer, _frames[i].indirectBuffer._allocation);
			vmaDestroyBuffer(_allocator, _frames[i].indirectCount._buffer, _frames[i].indirectCount._allocation);
			vmaDestroyBuffer(_allocator, _frames[i].clustersBuffer._buffer, _frames[i].clustersBuffer._allocation);
			vmaDestroyBuffer(_allocator, _frames[i].lightIndicesBuffer._buffer, _frames[i].lightIndicesBuffer._allocation);
		}
	});

}

size_t Renderer::padUniformBufferSize(size_t originalSize) {
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = _gpuProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}

AllocatedBuffer Renderer::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = allocSize,
		.usage = usage
	};

	VmaAllocationCreateInfo vmaallocInfo = {
		.usage = memoryUsage
	};

	//allocate the buffer
	AllocatedBuffer newBuffer;
	vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
		&newBuffer._buffer,
		&newBuffer._allocation,
		nullptr);

	return newBuffer;
}

void Renderer::initPipelines() {

	VkShaderModule meshVertShader;
	if (!loadShaderModule("../shaders/tri_mesh.vert.spv", meshVertShader)) {
		std::cout << "Error when building the triangle vertex shader module" << std::endl;
	} else {
		std::cout << "Mesh Triangle vertex shader successfully loaded" << std::endl;
	}

	VkShaderModule triangleFragShader;
	if (!loadShaderModule("../shaders/default_lit.frag.spv", triangleFragShader)) {
		std::cout << "Error when building the triangle fragment shader module" << std::endl;
	} else {
		std::cout << "Triangle fragment shader successfully loaded" << std::endl;
	}

	VkShaderModule texturedMeshShader;
	if (!loadShaderModule("../shaders/textured_lit.frag.spv", texturedMeshShader)) {
		std::cout << "Error when building the textured fragment shader module" << std::endl;
	} else {
		std::cout << "Textured fragment shader successfully loaded" << std::endl;
	}

	VkShaderModule specularMapShader;
	if (!loadShaderModule("../shaders/specular_map.frag.spv", specularMapShader)) {
		std::cout << "Error when building the textured fragment shader module" << std::endl;
	} else {
		std::cout << "Specular fragment shader successfully loaded" << std::endl;
	}

	//setup push constants
	VkPushConstantRange push_constant{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(MeshPushConstants)
	};

	std::vector<VkDescriptorSetLayout> setLayouts = { _globalSetLayout, _objectSetLayout };
	std::vector<VkPushConstantRange> pushConstants = { push_constant };

	//we start from just the default empty pipeline layout info
	VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info(setLayouts, pushConstants);

	VkPipelineLayout meshPipelineLayout;

	vkCreatePipelineLayout(_device, &mesh_pipeline_layout_info, nullptr, &meshPipelineLayout);

	auto vertexDescription = Vertex::getVertexBinding();

	amaz::eng::PipelineBuilder meshPipelineBuilder;
	meshPipelineBuilder.addShader({amaz::eng::ShaderStages::VERTEX, meshVertShader})
		.addShader({amaz::eng::ShaderStages::FRAGMENT, triangleFragShader})
		.addVertexBinding(vertexDescription)
		.setTopology(amaz::eng::Topology::TRIANGLE_LIST)
		.setViewport({0.0f, 0.0f, (float)_winSize.width, (float)_winSize.height, 0.0f, 1.0f})
		.setScissor({{ 0, 0 },  {(uint32_t)_winSize.width, (uint32_t)_winSize.height}})
		.setCullmode(amaz::eng::CullingMode::BACK)
		.setFacing(amaz::eng::PrimitiveFacing::COUNTER_CLOCKWISE)
		.disableRasterizerDiscard()
		.disableWireframe()
		.enableDepthBias(0, 0, 0)
		.disableDepthClamp()
		.addDynamicState(amaz::eng::DynamicPipelineState::DEPTH_BIAS | amaz::eng::DynamicPipelineState::SCISSOR | amaz::eng::DynamicPipelineState::VIEWPORT)
		.disableColorBlending() // doesn't do anything currently
		.setMSAASamples(amaz::eng::MSAASamples::ONE)
		.disableSampleShading()
		.setDepthInfo(true, false, amaz::eng::CompareOp::GREATER_OR_EQUAL);

	VkPipeline meshPipeline = backendRenderer->buildPipeline(meshPipelineBuilder, _mainRenderPass, meshPipelineLayout);


	createMaterial(meshPipeline, meshPipelineLayout, "defaultmesh");

	_prePassPipelineLayout = meshPipelineLayout;


	auto prePassPipelineBuilder = meshPipelineBuilder;
	prePassPipelineBuilder.clearShaders()
		.addShader({amaz::eng::ShaderStages::VERTEX, meshVertShader})
		.setDepthInfo(true, true, amaz::eng::CompareOp::GREATER_OR_EQUAL);

	_prePassPipeline = backendRenderer->buildPipeline(prePassPipelineBuilder, _prePassRenderPass, _prePassPipelineLayout);

	setLayouts = { _globalSetLayout, _objectSetLayout, _singleTextureSetLayout };

	VkPipelineLayoutCreateInfo textured_pipeline_layout_info =
		vkinit::pipeline_layout_create_info(setLayouts, pushConstants);

	VkPipelineLayout texturedPipeLayout;
	vkCreatePipelineLayout(_device, &textured_pipeline_layout_info, nullptr, &texturedPipeLayout);

	auto texturedPipelineBuilder = meshPipelineBuilder;
	texturedPipelineBuilder.clearShaders()
		.addShader({amaz::eng::ShaderStages::VERTEX, meshVertShader})
		.addShader({amaz::eng::ShaderStages::FRAGMENT, texturedMeshShader})
		.setDepthInfo(true, false, amaz::eng::CompareOp::GREATER_OR_EQUAL);

	VkPipeline texPipeline = backendRenderer->buildPipeline(texturedPipelineBuilder, _mainRenderPass, texturedPipeLayout);


	createMaterial(texPipeline, texturedPipeLayout, "texturedmesh");

	setLayouts = { _globalSetLayout, _objectSetLayout, _singleTextureSetLayout, _singleTextureSetLayout };

	VkPipelineLayoutCreateInfo spec_pipeline_layout_info =
		vkinit::pipeline_layout_create_info(setLayouts, pushConstants);

	VkPipelineLayout specPipeLayout;
	vkCreatePipelineLayout(_device, &spec_pipeline_layout_info, nullptr, &specPipeLayout);

	auto specPipelineBuilder = meshPipelineBuilder;
	specPipelineBuilder.clearShaders()
		.addShader({amaz::eng::ShaderStages::VERTEX, meshVertShader})
		.addShader({amaz::eng::ShaderStages::FRAGMENT, specularMapShader})
		.setDepthInfo(true, false, amaz::eng::CompareOp::GREATER_OR_EQUAL);
	
	VkPipeline specPipeline = backendRenderer->buildPipeline(specPipelineBuilder, _mainRenderPass, specPipeLayout);

	createMaterial(specPipeline, specPipeLayout, "specularmap");

	VkShaderModule shadowVertShader;
	if (!loadShaderModule("../shaders/shadow.vert.spv", shadowVertShader)) {
		std::cout << "Error when building the shader vertex shader module" << std::endl;
	}
	else {
		std::cout << "Shadow vertex shader successfully loaded" << std::endl;
	}

	VkShaderModule shadowFragShader;
	if (!loadShaderModule("../shaders/shadow.frag.spv", shadowFragShader)) {
		std::cout << "Error when building the shader fragment shader module" << std::endl;
	}
	else {
		std::cout << "Shadow fragment shader successfully loaded" << std::endl;
	}

	std::vector<VkDescriptorSetLayout> shadowSetLayouts = { _objectSetLayout };

	VkPushConstantRange shadowPushConstant{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(GPUShadowPushConstants)
	};
	std::vector<VkPushConstantRange> shadowPushConstants = { shadowPushConstant };

	VkPipelineLayoutCreateInfo shadowPipelineLayoutCreateInfo = vkinit::pipeline_layout_create_info(shadowSetLayouts, shadowPushConstants);

	vkCreatePipelineLayout(_device, &shadowPipelineLayoutCreateInfo, nullptr, &_shadowPipelineLayout);

	auto shadowPipelineBuilder = meshPipelineBuilder;
	shadowPipelineBuilder.clearShaders()
		.addShader({amaz::eng::ShaderStages::VERTEX, shadowVertShader})
		.addShader({amaz::eng::ShaderStages::FRAGMENT, shadowFragShader})
		.setViewport({0.0f, 0.0f, (float)8192, (float)8192, 0.0f, 1.0f})
		.setScissor({{ 0, 0 },  {(uint32_t)8192, (uint32_t)8192}})
		.setCullmode(amaz::eng::CullingMode::FRONT)
		.setDepthInfo(true, true, amaz::eng::CompareOp::LESS_OR_EQUAL);
	
	_shadowPipeline = backendRenderer->buildPipeline(shadowPipelineBuilder, _shadowRenderPass, _shadowPipelineLayout);

	VkShaderModule fullscreenVertShader;
	if (!loadShaderModule("../shaders/fullscreen.vert.spv", fullscreenVertShader)) {
		std::cout << "Error when building the fullscreen vertex shader module" << std::endl;
	}
	else {
		std::cout << "Tonemap vertex shader successfully loaded" << std::endl;
	}

	VkShaderModule tonemapFragShader;
	if (!loadShaderModule("../shaders/tonemap.frag.spv", tonemapFragShader)) {
		std::cout << "Error when building the tonemap fragment shader module" << std::endl;
	}
	else {
		std::cout << "Tonemap fragment shader successfully loaded" << std::endl;
	}

	std::vector<VkDescriptorSetLayout> tonemapSetLayouts = { _singleTextureSetLayout };

	VkPushConstantRange tonemapPushConstant{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(float)
	};
	std::vector<VkPushConstantRange> tonemapPushConstants = { tonemapPushConstant };

	VkPipelineLayoutCreateInfo tonemapPipelineLayoutCreateInfo = vkinit::pipeline_layout_create_info(tonemapSetLayouts, tonemapPushConstants);

	vkCreatePipelineLayout(_device, &tonemapPipelineLayoutCreateInfo, nullptr, &_tonemapPipelineLayout);

	amaz::eng::PipelineBuilder tonemapPipelineBuilder;

	tonemapPipelineBuilder.addShader({amaz::eng::ShaderStages::VERTEX, fullscreenVertShader})
		.addShader({amaz::eng::ShaderStages::FRAGMENT, tonemapFragShader})
		.setTopology(amaz::eng::Topology::TRIANGLE_LIST)
		.setViewport({0.0f, 0.0f, (float)_winSize.width, (float)_winSize.height, 0.0f, 1.0f})
		.setScissor({{ 0, 0 },  {(uint32_t)_winSize.width, (uint32_t)_winSize.height}})
		.setCullmode(amaz::eng::CullingMode::NONE)
		.disableRasterizerDiscard()
		.disableWireframe()
		.disableDepthBias()
		.addDynamicState(amaz::eng::DynamicPipelineState::SCISSOR | amaz::eng::DynamicPipelineState::VIEWPORT)
		.disableColorBlending()
		.setMSAASamples(amaz::eng::MSAASamples::ONE)
		.disableSampleShading()
		.setDepthInfo(false, false, amaz::eng::CompareOp::GREATER_OR_EQUAL);

	
	_tonemapPipeline = backendRenderer->buildPipeline(tonemapPipelineBuilder, _tonemapRenderPass, _tonemapPipelineLayout);

	_mainFrameImagesSets = std::vector<VkDescriptorSet>(_swapchainImages.size());

	for (int i = 0; i < _swapchainImages.size(); i++) {
		VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);

		VkSampler sampler;
		vkCreateSampler(_device, &samplerInfo, nullptr, &sampler);

		VkDescriptorImageInfo imageBufferInfo{
			.sampler = sampler,
			.imageView = _mainFrameImageViews[i],
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		auto mainFrameDescriptorSet = amaz::eng::DescriptorBuilder::init()
			.add_image(0, 1, amaz::eng::BindingType::IMAGE_SAMPLER, amaz::eng::ShaderStages::FRAGMENT, imageBufferInfo);
		_mainFrameImagesSets[i] = mainFrameDescriptorSet.build_set(_device, _descriptorPool, _singleTextureSetLayout);

	}

	//deleting all of the vulkan shaders
	vkDestroyShaderModule(_device, meshVertShader, nullptr);
	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, texturedMeshShader, nullptr);
	vkDestroyShaderModule(_device, specularMapShader, nullptr);
	vkDestroyShaderModule(_device, shadowVertShader, nullptr);
	vkDestroyShaderModule(_device, fullscreenVertShader, nullptr);
	vkDestroyShaderModule(_device, tonemapFragShader, nullptr);

	//adding the pipelines to the deletion queue
	_mainDeletionQueue.push_function([=]() {
		vkDestroyPipeline(_device, meshPipeline, nullptr);
		vkDestroyPipeline(_device, texPipeline, nullptr);
		vkDestroyPipeline(_device, specPipeline, nullptr);
		vkDestroyPipeline(_device, _shadowPipeline, nullptr);
		vkDestroyPipeline(_device, _tonemapPipeline, nullptr);

		vkDestroyPipelineLayout(_device, meshPipelineLayout, nullptr);
		vkDestroyPipelineLayout(_device, texturedPipeLayout, nullptr);
		vkDestroyPipelineLayout(_device, specPipeLayout, nullptr);
		vkDestroyPipelineLayout(_device, _shadowPipelineLayout, nullptr);
		vkDestroyPipelineLayout(_device, _tonemapPipelineLayout, nullptr);
		});

	initComputePipelines();
}

VkPipeline Renderer::initComputePipeline(std::string filePath, VkPipelineLayout pipelineLayoutInfo) {

	VkShaderModule shaderModule;
	if (!loadShaderModule(filePath, shaderModule)) {
		std::cout << "Error when building shader module, filepath: " << filePath << std::endl;
	} else {
		std::cout << filePath << " shader successfully loaded" << std::endl;
	}

	VkComputePipelineCreateInfo pipelineCreateInfo {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,

		.flags = 0,
		.stage = vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, shaderModule),
		.layout = pipelineLayoutInfo
	};

	VkPipeline pipeline;
	vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);

	//deleting the compute shader
	vkDestroyShaderModule(_device, shaderModule, nullptr);

	//adding the pipeline to the deletion queue
	_mainDeletionQueue.push_function([=]() {
		vkDestroyPipeline(_device, pipeline, nullptr);
	});

	return pipeline;
}

// Create set containing:
// binding #0: depth sampler
// binding #1: pre-pass depth buffer (sampled image)
// binding #2: depth pyramid sampled image (read-only sampled access)
// binding #3: depth pyramid storage image (write access) (array of all mipmaps, 11)
void Renderer::createPyramidSetLayout() {
	uint32_t depthPyramidMipLevels = _depthPyramidViews.size(); // 11
	
	auto pyramidDescriptorLayoutBuilder = amaz::eng::DescriptorBuilder::init()
		.add_image(0, 1, amaz::eng::BindingType::SAMPLER, amaz::eng::ShaderStages::COMPUTE, std::nullopt, &_depthSampler)
		.add_image(1, 1, amaz::eng::BindingType::SAMPLED_IMAGE, amaz::eng::ShaderStages::COMPUTE)
		.add_image(2, 1, amaz::eng::BindingType::SAMPLED_IMAGE, amaz::eng::ShaderStages::COMPUTE)
		.add_image(3, depthPyramidMipLevels, amaz::eng::BindingType::STORAGE_IMAGE, amaz::eng::ShaderStages::COMPUTE);

	_depthPyramidSetLayout = pyramidDescriptorLayoutBuilder.build_layout(_device);

	_mainDeletionQueue.push_function([&]() {
		vkDestroyDescriptorSetLayout(_device, _depthPyramidSetLayout, nullptr);
	});

	VkDescriptorImageInfo sampledPyramidInfo {
		.imageView = _depthPyramidView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};
	
	std::vector<VkDescriptorImageInfo> storagePyramidInfo;

	for (uint32_t i = 0; i < depthPyramidMipLevels; i++) {
		storagePyramidInfo.push_back({
			.imageView = _depthPyramidViews[i],
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL
		});
	}

	for (uint32_t i = 0; i < _mainFrameDepthImageViews.size(); i++) {
		VkDescriptorImageInfo depthInfo {
			.imageView = _mainFrameDepthImageViews[i],
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		auto pyramidDescriptorLayoutBuilder = amaz::eng::DescriptorBuilder::init()
			//.add_image(0, 1, amaz::eng::BindingType::SAMPLER, amaz::eng::ShaderStages::COMPUTE, std::nullopt, &_depthSampler)
			.add_image(1, 1, amaz::eng::BindingType::SAMPLED_IMAGE, amaz::eng::ShaderStages::COMPUTE, depthInfo)
			.add_image(2, 1, amaz::eng::BindingType::SAMPLED_IMAGE, amaz::eng::ShaderStages::COMPUTE, sampledPyramidInfo)
			.add_image(3, depthPyramidMipLevels, amaz::eng::BindingType::STORAGE_IMAGE, amaz::eng::ShaderStages::COMPUTE, storagePyramidInfo.data());
			
		VkDescriptorSet depthSet = pyramidDescriptorLayoutBuilder.build_set(_device, _descriptorPool, _depthPyramidSetLayout);

		_depthPyramidSets.push_back(depthSet);
	}
	
}

uint32_t previousPow2(uint32_t v) {
	uint32_t r = 1;

	while (r * 2 < v)
		r *= 2;

	return r;
}

uint32_t getImageMipLevels(uint32_t width, uint32_t height) {
	uint32_t result = 1;

	while (width > 1 || height > 1)
	{
		result++;
		width /= 2;
		height /= 2;
	}

	return result;
}

void Renderer::createDepthPyramidImage() {

	uint32_t depthPyramidWidth = previousPow2(_winSize.width);
	uint32_t depthPyramidHeight = previousPow2(_winSize.height);

	depthPyramidWidth = 1024;
	depthPyramidHeight = 512;

	uint32_t depthPyramidMipLevels = getImageMipLevels(depthPyramidWidth, depthPyramidHeight);
	std::cout << depthPyramidMipLevels << "\n";

	VkExtent3D extent{
		.width = depthPyramidWidth,
		.height = depthPyramidHeight,
		.depth = 1
	};

	VmaAllocationCreateInfo allocInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	auto depthPyramidImageInfo = vkinit::image_create_info(VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, extent, 0, 1, depthPyramidMipLevels);

	vmaCreateImage(_allocator, &depthPyramidImageInfo, &allocInfo, &_depthPyramid._image, &_depthPyramid._allocation, nullptr);

	immediateSubmit([&](VkCommandBuffer cmd) {
			VkImageSubresourceRange range{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = depthPyramidMipLevels,
				.baseArrayLayer = 0,
				.layerCount = 1
			};

			VkImageMemoryBarrier imageBarrier_toShaderRead{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

				.srcAccessMask = VK_ACCESS_NONE,
				.dstAccessMask = VK_ACCESS_NONE,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.image = _depthPyramid._image,
				.subresourceRange = range,
			};

			//barrier the image into the layout
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toShaderRead);
		});

	_depthPyramidViews.resize(depthPyramidMipLevels);

	auto viewInfo = vkinit::imageview_create_info(VK_FORMAT_R32_SFLOAT, _depthPyramid._image, VK_IMAGE_ASPECT_COLOR_BIT);
	viewInfo.subresourceRange.levelCount = depthPyramidMipLevels;
	viewInfo.subresourceRange.baseMipLevel = 0;

	vkCreateImageView(_device, &viewInfo, nullptr, &_depthPyramidView);
	
	for (uint32_t i = 0; i < depthPyramidMipLevels; i++) {
		//auto viewInfo = vkinit::imageview_create_info(VK_FORMAT_R32_SFLOAT, _depthPyramid._image, VK_IMAGE_ASPECT_COLOR_BIT);
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseMipLevel = i;

		vkCreateImageView(_device, &viewInfo, nullptr, &_depthPyramidViews[i]);
	}

	_mainDeletionQueue.push_function([=]() {
		vmaDestroyImage(_allocator, _depthPyramid._image, _depthPyramid._allocation);
		for (auto depthView : _depthPyramidViews)
			vkDestroyImageView(_device, depthView, nullptr);
	});

	VkSamplerReductionModeCreateInfoEXT createInfoReduction {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT,

		.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN
	};

	VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = &createInfoReduction,

		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,

		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,

		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,

		.minLod = 0,
		.maxLod = 16.f,
	};

	vkCreateSampler(_device, &samplerInfo, nullptr, &_depthSampler);
}

void Renderer::initComputePipelines() {

	createDepthPyramidImage();
	createPyramidSetLayout();

	//setup push constants
	VkPushConstantRange pushConstant{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof(GPULightCullPushConstants)
	};

	std::vector<VkDescriptorSetLayout> setLayouts = { _globalSetLayout, _objectSetLayout, _depthPyramidSetLayout };

	std::vector<VkPushConstantRange> pushConstants = { pushConstant };

	VkPipelineLayoutCreateInfo lightCullPipelineLayoutInfo = vkinit::pipeline_layout_create_info(setLayouts, pushConstants);

	vkCreatePipelineLayout(_device, &lightCullPipelineLayoutInfo, nullptr, &_lightCullPipelineLayout);

	_lightCullPipeline = initComputePipeline("../shaders/cullLights.comp.spv", _lightCullPipelineLayout);



	std::vector<VkDescriptorSetLayout> clusterSetLayouts = { _globalSetLayout, _objectSetLayout };
	std::vector<VkPushConstantRange> clusterPushConstants = { {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof(GPUClusterPushConstant)
	} };

	VkPipelineLayoutCreateInfo clusterPipelineLayoutInfo = vkinit::pipeline_layout_create_info(clusterSetLayouts, clusterPushConstants);
	vkCreatePipelineLayout(_device, &clusterPipelineLayoutInfo, nullptr, &_clusterPipelineLayout);

	_clusterPipeline = initComputePipeline("../shaders/clusterLightCull.comp.spv", _clusterPipelineLayout);



	std::array<VkDescriptorSetLayout, 1> depthSetLayouts = { _depthPyramidSetLayout };

	VkPushConstantRange depthPushConstant{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof(GPUDepthReducePushConstants)
	};

	std::array<VkPushConstantRange, 1> depthPushConstants = { depthPushConstant };

	VkPipelineLayoutCreateInfo depthPyramidPipelineLayoutInfo = vkinit::pipeline_layout_create_info(depthSetLayouts, depthPushConstants);

	vkCreatePipelineLayout(_device, &depthPyramidPipelineLayoutInfo, nullptr, &_depthPyramidPipelineLayout);

	_depthPyramidPipeline = initComputePipeline("../shaders/depthReduce.comp.spv", _depthPyramidPipelineLayout);



	//adding the pipelines to the deletion queue
	_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(_device, _lightCullPipelineLayout, nullptr);
		vkDestroyPipelineLayout(_device, _depthPyramidPipelineLayout, nullptr);
	});
}

bool Renderer::loadShaderModule(std::string filePath, VkShaderModule& outShaderModule) {
	//open the file. With cursor at the end
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	//find what the size of the file is by looking up the location of the cursor
	//because the cursor is at the end, it gives the size directly in bytes
	size_t fileSize = (size_t)file.tellg();

	//spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	//put file cursor at beginning
	file.seekg(0);

	//load the entire file into the buffer
	file.read((char*)buffer.data(), fileSize);

	//now that the file is loaded into the buffer, we can close it
	file.close();

	//create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,

		//codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
		.codeSize = buffer.size() * sizeof(uint32_t),
		.pCode = buffer.data()
	};

	//check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}
	outShaderModule = shaderModule;
	return true;
}

Material& Renderer::createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name) {
	Material mat{
		.pipeline = pipeline,
		.pipelineLayout = layout
	};
	_materials[name] = mat;
	return _materials[name];
}

void Renderer::registerMaterial(std::string matTemplate, std::string name, std::optional<std::string> diffuseMap, std::optional<std::string> specularMap) {



	Material* tempMat = getMaterial(matTemplate);

	Material mat{
		.pipeline = tempMat->pipeline,
		.pipelineLayout = tempMat->pipelineLayout
	};

	if (diffuseMap.has_value()) {
		if (specularMap.has_value()) {
			// Not implemented
		}
		VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);

		VkSampler sampler;
		vkCreateSampler(_device, &samplerInfo, nullptr, &sampler);

		VkDescriptorImageInfo imageBufferInfo{
			.sampler = sampler,
			.imageView = _loadedTextures[diffuseMap.value()].imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		auto diffuseMapSetBuilder = amaz::eng::DescriptorBuilder::init()
			.add_image(0, 1, amaz::eng::BindingType::IMAGE_SAMPLER, amaz::eng::ShaderStages::FRAGMENT, imageBufferInfo);

		mat.textureSet = diffuseMapSetBuilder.build_set(_device, _descriptorPool, _singleTextureSetLayout);

	}

	_materials[name] = mat;
	//return _materials[name];
}

void Renderer::initImgui() {

	// create descriptor pool for IMGUI
	// TODO: maybe make this smaller lol
	std::array<VkDescriptorPoolSize, 11> pool_sizes{
		{{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }}
	};

	VkDescriptorPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

		.maxSets = 1000,
		.poolSizeCount = pool_sizes.size(),
		.pPoolSizes = pool_sizes.data()
	};

	VkDescriptorPool imguiPool;

	vkCreateDescriptorPool(_device, &poolInfo, nullptr, &imguiPool);

	// initialize imgui library
	ImGui::CreateContext();

	ImGui_ImplSDL2_InitForVulkan(_window.get());

	ImGui_ImplVulkan_InitInfo initInfo{
		.Instance = _instance,
		.PhysicalDevice = _physicalDevice,
		.Device = _device,
		.Queue = _graphicsQueue,
		.DescriptorPool = imguiPool,
		.MinImageCount = 3,
		.ImageCount = 3,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT
	};

	ImGui_ImplVulkan_Init(&initInfo, _tonemapRenderPass);


	immediateSubmit([&](VkCommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	_mainDeletionQueue.push_function([=]() {

		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
		});

}

void Renderer::loadImage(std::string filename, std::string textureName) {
	Texture texture;
	loadImageFromFile(filename, texture.image);

	VkImageViewCreateInfo imageinfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, texture.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCreateImageView(_device, &imageinfo, nullptr, &texture.imageView);

	_loadedTextures[textureName] = texture;
}

bool Renderer::loadImageFromFile(std::string file, AllocatedImage& outImage) {
	int texWidth, texHeight, texChannels;

	stbi_uc* pixels = stbi_load(file.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		std::cout << "Failed to load texture file " << file << std::endl;
		return false;
	}

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	//the format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
	VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

	//allocate temporary buffer for holding texture data to upload
	AllocatedBuffer stagingBuffer = createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	//copy data to buffer
	void* data;
	vmaMapMemory(_allocator, stagingBuffer._allocation, &data);

	memcpy(data, pixels, static_cast<size_t>(imageSize));

	vmaUnmapMemory(_allocator, stagingBuffer._allocation);
	//we no longer need the loaded data, so we can free the pixels as they are now in the staging buffer
	stbi_image_free(pixels);

	VkExtent3D imageExtent{
		.width = static_cast<uint32_t>(texWidth),
		.height = static_cast<uint32_t>(texHeight),
		.depth = 1
	};

	VkImageCreateInfo dimg_info = vkinit::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

	AllocatedImage newImage;

	VmaAllocationCreateInfo dimg_allocinfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY
	};

	//allocate and create the image
	vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

	//transition and copy
	immediateSubmit([&](VkCommandBuffer cmd) {
		VkImageSubresourceRange range{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		VkImageMemoryBarrier imageBarrier_toTransfer{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.image = newImage._image,
			.subresourceRange = range,
		};

		//barrier the image into the transfer-receive layout
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

		VkImageSubresourceLayers subresourceLayers{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};

		VkBufferImageCopy copyRegion = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,

			.imageSubresource = subresourceLayers,
			.imageExtent = imageExtent,
		};

		//copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		VkImageMemoryBarrier imageBarrier_toReadable{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.image = newImage._image,
			.subresourceRange = range,
		};

		//barrier the image into the shader readable layout
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

		});

	_mainDeletionQueue.push_function([=]() {
		vmaDestroyImage(_allocator, newImage._image, newImage._allocation);
		});

	vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);

	std::cout << "Texture loaded successfully " << file << std::endl;

	outImage = newImage;


	return true;
}

void Renderer::loadMesh(std::string name, std::string filename) {
	Mesh mesh{};
	mesh.load_from_obj(filename);
	uploadMesh(mesh);
	_meshes[name] = mesh;
}

void Renderer::loadLight(glm::vec3 pos, glm::vec3 color, float radius) {
	PointLightObject light{
		.lightPos = pos,
		.lightColor = color,
		.ambientColor = color * 0.25f,
		.radius = radius
	};
	_pointLights.push_back(light);
}

std::vector<Vertex> Renderer::createCuboid(float x, float y, float z) {
	x = x / 2;
	y = y / 2;
	z = z / 2;
	glm::vec3 topLeftFront = { -x, y, z };
	glm::vec3 topRightFront = { x, y, z };
	glm::vec3 bottomLeftFront = { -x, -y, z };
	glm::vec3 bottomRightFront = { x, -y, z };
	glm::vec3 topLeftBack = { -x, y, -z };
	glm::vec3 topRightBack = { x, y, -z };
	glm::vec3 bottomLeftBack = { -x, -y, -z };
	glm::vec3 bottomRightBack = { x, -y, -z };

	auto frontFace = createSquare(topLeftFront, topRightFront, bottomLeftFront, bottomRightFront, { 0, 0, 1 });
	auto backFace = createSquare(topRightBack, topLeftBack, bottomRightBack, bottomLeftBack, { 0, 0, -1 });

	auto topFace = createSquare(topLeftBack, topRightBack, topLeftFront, topRightFront, { 0, 1, 0 });
	auto bottomFace = createSquare(bottomLeftFront, bottomRightFront, bottomLeftBack, bottomRightBack, { 0, -1, 0 });

	auto rightFace = createSquare(topRightFront, topRightBack, bottomRightFront, bottomRightBack, { 1, 0, 0 });
	auto leftFace = createSquare(topLeftBack, topLeftFront, bottomLeftBack, bottomLeftFront, { -1, 0, 0 });

	std::vector<std::vector<Vertex>> faces = {
		frontFace, backFace, topFace, bottomFace, rightFace, leftFace
	};

	std::vector<Vertex> cube(36);
	for (auto face : faces) {
		cube.insert(cube.end(), face.begin(), face.end());
	}

	return cube;
}

std::vector<Vertex> Renderer::createCube() {


	glm::vec3 topLeftFront = { -1.f, 1.f, 1.f };
	glm::vec3 topRightFront = { 1.f, 1.f, 1.f };
	glm::vec3 bottomLeftFront = { -1.f, -1.f, 1.f };
	glm::vec3 bottomRightFront = { 1.f, -1.f, 1.f };
	glm::vec3 topLeftBack = { -1.f, 1.f, -1.f };
	glm::vec3 topRightBack = { 1.f, 1.f, -1.f };
	glm::vec3 bottomLeftBack = { -1.f, -1.f, -1.f };
	glm::vec3 bottomRightBack = { 1.f, -1.f, -1.f };

	auto frontFace = createSquare(topLeftFront, topRightFront, bottomLeftFront, bottomRightFront, { 0, 0, 1 });
	auto backFace = createSquare(topRightBack, topLeftBack, bottomRightBack, bottomLeftBack, { 0, 0, -1 });

	auto topFace = createSquare(topLeftBack, topRightBack, topLeftFront, topRightFront, { 0, 1, 0 });
	auto bottomFace = createSquare(bottomLeftFront, bottomRightFront, bottomLeftBack, bottomRightBack, { 0, -1, 0 });

	auto rightFace = createSquare(topRightFront, topRightBack, bottomRightFront, bottomRightBack, { 1, 0, 0 });
	auto leftFace = createSquare(topLeftBack, topLeftFront, bottomLeftBack, bottomLeftFront, { -1, 0, 0 });

	std::vector<std::vector<Vertex>> faces = {
		frontFace, backFace, topFace, bottomFace, rightFace, leftFace
	};

	std::vector<Vertex> cube(36);
	for (auto face : faces) {
		cube.insert(cube.end(), face.begin(), face.end());
	}

	return cube;
}

std::vector<Vertex> Renderer::createSquare(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight, glm::vec3 normal, glm::vec3 color) {
	return {
		{.position = topLeft, .normal = normal, .color = color, .uv = {0.f, 1.f} },
		{.position = bottomLeft, .normal = normal, .color = color, .uv = {0.f, 0.f} },
		{.position = topRight, .normal = normal, .color = color, .uv = {1.f, 1.f} },
		{.position = topRight, .normal = normal, .color = color, .uv = {1.f, 1.f} },
		{.position = bottomLeft, .normal = normal, .color = color, .uv = {0.f, 0.f} },
		{.position = bottomRight, .normal = normal, .color = color, .uv = {1.f, 0.f} }
	};
}

void Renderer::uploadMesh(Mesh& mesh) {
	createStageAndCopyBuffer(std::span<Vertex>(mesh._vertices), mesh._vertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	createStageAndCopyBuffer(std::span<uint32_t>(mesh._indices), mesh._indexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

template <typename T>
void Renderer::createStageAndCopyBuffer(std::span<T> data, AllocatedBuffer& bufferLocation, VkBufferUsageFlags usageFlags) {
	const size_t bufferSize = data.size() * sizeof(T);

	//allocate staging buffer
	VkBufferCreateInfo stagingBufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,

		.size = bufferSize,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
	};

	//let the VMA library know that this data should be on CPU RAM
	VmaAllocationCreateInfo vmaallocInfo = {
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO
	};

	AllocatedBuffer stagingBuffer;

	//allocate the buffer
	vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo,
		&stagingBuffer._buffer,
		&stagingBuffer._allocation,
		nullptr);


	void* dataPtr;
	vmaMapMemory(_allocator, stagingBuffer._allocation, &dataPtr);

	memcpy(dataPtr, data.data(), bufferSize);

	vmaUnmapMemory(_allocator, stagingBuffer._allocation);

	//allocate buffer
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.size = bufferSize,
		.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	};

	vmaallocInfo = {
		.flags = 0,
		.usage = VMA_MEMORY_USAGE_AUTO
	};

	//allocate the buffer
	vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
		&bufferLocation._buffer,
		&bufferLocation._allocation,
		nullptr);

	immediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy copy{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = bufferSize
		};
		vkCmdCopyBuffer(cmd, stagingBuffer._buffer, bufferLocation._buffer, 1, &copy);
		});

	

	_mainDeletionQueue.push_function([=]() {
		vmaDestroyBuffer(_allocator, bufferLocation._buffer, bufferLocation._allocation);
		});

	vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

void Renderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {
	VkCommandBuffer cmd = _uploadContext._commandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkBeginCommandBuffer(cmd, &cmdBeginInfo);

	//execute the function
	function(cmd);

	vkEndCommandBuffer(cmd);

	VkSubmitInfo submit = vkinit::submit_info(&cmd);


	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence);

	vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
	vkResetFences(_device, 1, &_uploadContext._uploadFence);

	// reset the command buffers inside the command pool
	vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

/*
	* Calculate a model -> world space transform matrix for a given position, scale and rotation
	*
	* @param position Position of the object within world space
	* @param scale Scale at which to transform the matrix
	* @rotation Rotation on each axis in degrees
	*
	* @return Transform matrix transforming from model to world space
	*/
glm::mat4 Renderer::calcTransformMatrix(glm::vec3 position, float scale, glm::vec3 rotation) {
	auto translation = glm::translate(position);

	auto scaleMatrix = glm::scale(glm::vec3(scale, scale, scale));

	glm::mat4 rotateX = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(rotation.x),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	glm::mat4 rotateY = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(rotation.y),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	glm::mat4 rotateZ = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(rotation.z),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	glm::mat4 rotate = rotateX * rotateY * rotateY;

	return translation * rotate * scaleMatrix;
}

void Renderer::registerRenderObject(std::string mesh, std::string material, glm::mat4 transform) {
	RenderObject object{
		.mesh = getMesh(mesh),
		.material = getMaterial(material),
		.transformMatrix = transform
	};

	_renderables.push_back(object);
}

void Renderer::registerRenderObject(std::string mesh, std::string material, glm::vec3 position) {
	registerRenderObject(mesh, material, glm::translate(position));
}

void Renderer::registerRenderObject(std::string mesh, std::string material, glm::vec3 position, float scale, glm::vec3 rotation) {

	glm::mat4 transformMatrix = calcTransformMatrix(position, scale, rotation);

	registerRenderObject(mesh, material, transformMatrix);
}

void Renderer::allocateTexture(VkImageView imageView, VkDescriptorPool descPool, VkDescriptorSetLayout* setLayout, VkDescriptorSet* textureSet) {
	VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);

	VkSampler sampler;
	vkCreateSampler(_device, &samplerInfo, nullptr, &sampler);

	VkDescriptorImageInfo imageBufferInfo{
		.sampler = sampler,
		.imageView = imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	auto textureSetBuilder = amaz::eng::DescriptorBuilder::init()
		.add_image(0, 1, amaz::eng::BindingType::IMAGE_SAMPLER, amaz::eng::ShaderStages::FRAGMENT, imageBufferInfo);

	*textureSet = textureSetBuilder.build_set(_device, descPool, *setLayout);

	_mainDeletionQueue.push_function([=]() {
		vkDestroySampler(_device, sampler, nullptr);
	});

}

// searches for the material, and return nullptr if not found
Material* Renderer::getMaterial(const std::string& name) {
	if (auto it = _materials.find(name); it != _materials.end())
		return &(it->second);
	else
		return nullptr;
}

// searches for the mesh, and return nullptr if not found
Mesh* Renderer::getMesh(const std::string& name) {
	if (auto it = _meshes.find(name); it != _meshes.end())
		return &(it->second);
	else
		return nullptr;
}

void Renderer::setPlayerPos(glm::vec3 pos) {
	renderPos = pos;
}

void Renderer::setThirdPerson(bool toggle) {
	thirdPerson = toggle;
}

//TODO: make this function work
void Renderer::setPlayerPosAndDir(glm::vec3 pos, glm::vec3 dir) {

	//glm::mat4 view = glm::lookAt(
	//	pos,
	//	pos + dir,
	//	glm::vec3(0, 1, 0)
	//);

	//_renderables[0].transformMatrix = glm::rotate(glm::translate(glm::mat4{ 1.0f }, pos), glm::radians(90.f), dir);
}
float depthBiasConstantFactor = 4.f;

float depthBiasSlopeFactor = 6.f;

int shadowSamples = 3;
float shadowStride = 1.0f;

float exposure = 1.f;

bool changeWinSize = 0;

int winSizeX = 1600;
int winSizeY = 900;

float lightRadius = 20.f;

std::vector<float> frameTimes;

float fps = 0;
int updateFps = 0;

float timeTillUpdateFps = 0.f;

bool depthPyramid = true;

void Renderer::drawImguiWindow(Input* input) {

	static int health = 20;

	constexpr float MAX_HEALTH = 200.f;

	ImGui::NewFrame();

	static bool configOpen;

	ImGui::Begin("Test Window");
	if (ImGui::CollapsingHeader("Config")) {
		ImGui::SliderFloat("fov", &fov, 50.f, 90.f);
		ImGui::SliderFloat("sens", &input->sens, 0.f, 50.f);
	}
	ImGui::Checkbox("noclip", &input->flying);
	if (ImGui::CollapsingHeader("Rendering")) {
		ImGui::SliderFloat("Depth Bias constant", &depthBiasConstantFactor, 0.f, 10.f);
		ImGui::SliderFloat("Depth Bias slope", &depthBiasSlopeFactor, 0.f, 10.f);

		ImGui::SliderInt("Shadow Samples", &shadowSamples, 1.f, 100.f);
		ImGui::SliderFloat("Shadow Stride", &shadowStride, 1.f, 100.f);

		ImGui::SliderFloat("Exposure", &exposure, 0.f, 10.f);

		ImGui::SliderFloat("Light radius", &lightRadius, 0.f, 100.f);
	}

	if (ImGui::CollapsingHeader("Resolution")) {
		ImGui::InputInt("Width", &winSizeX);
		ImGui::InputInt("Height", &winSizeY);
		if (ImGui::Button("Set Resolution")) {
			_winSize = {(uint32_t)winSizeX, (uint32_t)winSizeY};
			recreateRenderBuffers();
		}
		if(ImGui::Checkbox("Vsync", &vsync)){
			recreateFrameBuffers(_actualWinSize.width, _actualWinSize.height);
		};
	}

	if(frameTimes.size() >= 100) {
		for (size_t i = 1; i < frameTimes.size(); i++) {
			frameTimes[i-1] = frameTimes[i];
		}
		frameTimes[frameTimes.size() - 1] = frametime;
	} else {
		frameTimes.push_back(frametime);
	}

	timeTillUpdateFps -= frametime;

	if (timeTillUpdateFps <= 0.f && frameTimes.size() > 20) {
		float totalFrameTime = 0.f;
		int j = 0;
		for (size_t i = frameTimes.size() - 1; i >= 0 && i >= frameTimes.size()-10; i--) {
			totalFrameTime+=frameTimes[i];
			j++;
			
		}

		totalFrameTime/=10.f;
		fps = 1000/totalFrameTime;
		
		timeTillUpdateFps = 100;
	}

	std::ostringstream fpsText;

	fpsText.precision(0);
	fpsText << "FPS: " << std::fixed << fps;

	std::ostringstream frameTimeText;

	frameTimeText.precision(2);
	frameTimeText << "Frametime: " << std::fixed << frametime << "ms";

	ImGui::TextUnformatted(fpsText.str().c_str());
	ImGui::TextUnformatted(frameTimeText.str().c_str());
	ImGui::PlotLines("Frametimes", frameTimes.data(), frameTimes.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(300, 100));

	ImGui::Checkbox("generate depth pyramid", &depthPyramid);

	ImGui::End();

	ImGui::Render();
}

glm::mat4 perspectiveProjectionMatrix(float fovY, float aspect, float zNear, float zFar, glm::mat4* inverse = nullptr) {
	float f = glm::tan(fovY / 2.f);

	float focal_length = 1.0f / std::tan(fovY / 2.0f);

	float x  =  focal_length / aspect;
    float y  = -focal_length;
    float A  = zNear / (zFar - zNear);
    float B  = zFar * A;

	if (inverse)
    {
        *inverse = {
            1/x,  0.0f, 0.0f,  0.0f,
            0.0f,  1/y, 0.0f,  0.0f,
            0.0f, 0.0f, 0.0f, 1/B,
            0.0f, 0.0f,  -1.0f,   A/B,
        };
    }

	return {
		x,		0.f,	0.f,	0.f,
		0.f,	y,		0.f,	0.f,
		0.f,	0.f,	A,		-1.f,
		0.f,	0.f,	B,		0.f
	};

	
}

void Renderer::draw(glm::vec3 camDir, Input* input) {

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(_window.get());
	drawImguiWindow(input);

	auto& frame = getCurrentFrame();
	//wait until the GPU has finished rendering the last frame. Timeout of 1 second
	vkWaitForFences(_device, 1, &frame._renderFence, true, 1000000000);
	vkResetFences(_device, 1, &frame._renderFence);

	//request image from the swapchain, one second timeout
	uint32_t swapchainImageIndex;

	auto result = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, frame._presentSemaphore, nullptr, &swapchainImageIndex);

	if (result == VK_TIMEOUT) {
		std::cout << "vkAcquireNextImageKHR timed out" << std::endl;
	}

	if (result != VK_SUCCESS) {
		std::cout << "vkAcquireNextImageKHR returned: " << result;
	}

	//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	vkResetCommandBuffer(frame._mainCommandBuffer, 0);

	//naming it cmd for shorter writing
	VkCommandBuffer cmd = frame._mainCommandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,

		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(cmd, &cmdBeginInfo);

	//make a clear-color from frame number. This will flash with a 120*pi frame period.
	float flash = abs(sin(_frameNumber / 240.f));
	VkClearValue clearValue{
		.color = { { 0.0f, 0.0f, 0.0f, 1.0f } }
	};

	//clear depth at 1
	VkClearValue depthClear{
		.depthStencil = {.depth = 0.f}
	};

	std::array<VkClearValue, 2> clearValues = { clearValue, depthClear };

	//start the main renderpass.
	//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	VkRenderPassBeginInfo rpInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,

		.renderPass = _mainRenderPass,
		.framebuffer = _mainFramebuffers[swapchainImageIndex],
		.renderArea{
			.offset = {0, 0},
			.extent = { (uint32_t)_winSize.width, (uint32_t)_winSize.height}
		},

		//connect clear values
		.clearValueCount = 2,
		.pClearValues = &clearValues[0]
	};

	//sortObjects(_renderables);

	glm::vec3 camPos;

	if (thirdPerson)
		camPos = renderPos - (camDir * glm::vec3{ 3 }) + glm::vec3{ 0, 1, 0 };
	else
		camPos = renderPos + glm::vec3{ 0, 0.6f, 0 };

	glm::vec3 sunlightDir = { -1.f, 2.f, -1.f };
	float near_plane = 1.0f, far_plane = 150.f;
	glm::mat4 skyLightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);

	glm::mat4 skyLightView = glm::lookAt(
		sunlightDir * 50.f,
		glm::vec3(0.f),
		glm::vec3(0, 1, 0)
	);

	glm::mat4 skyLightMatrix = skyLightProjection * skyLightView;

	if (shadowSamples <= 0)
		shadowSamples = 1;

	if (shadowStride <= 0)
		shadowStride = 1;
	

	GPUSceneData sceneParameters = {
		.ambientColor = { 0.1f, 0.1f, 0.1f },
		.sunlightDirection = sunlightDir,
		.sunlightColor = { 0.8f, 0.8f, 0.8f},
		.camPos = camPos,
		.lightSpaceMatrix = skyLightMatrix,
		.shadowSamples = shadowSamples,
		.shadowStride = shadowStride,
		.farPlane = 200.f,
		.nearPlane = 0.1f
	};

	
	//camera view
	glm::mat4 camView = glm::lookAt(
		camPos,
		camPos + camDir,
		glm::vec3(0, 1, 0)
	);

	float zNear = 0.1f;
	float zFar = 200.f;
	//camera projection
	glm::mat4 camProj;
	glm::mat4 inverseCamProj;
	camProj = perspectiveProjectionMatrix(glm::radians(fov), 1600.f / 900.f, zNear, zFar, &inverseCamProj);

	mapData(_renderables, camView, camProj, sceneParameters, _pointLights);

	drawPrePass(cmd, _renderables, sceneParameters, swapchainImageIndex);

	if (depthPyramid) 
		genDepthPyramid(cmd, swapchainImageIndex);

	cullLightsPass(cmd, _pointLights, camView, camProj, swapchainImageIndex, zNear, zFar);

	clusterLightsPass(cmd, true, camView, inverseCamProj, zNear, zFar);

	static bool hasShadows = false;

	if (!hasShadows) {
		drawShadowPass(cmd, _renderables, sceneParameters, _pointLights);
		hasShadows = true;
	}

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = (float)_winSize.width,
		.height = (float)_winSize.height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	VkRect2D scissor = {
		.offset = {0,0},
		.extent = { (uint32_t)_winSize.width, (uint32_t)_winSize.height}
	};

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	vkCmdSetDepthBias(cmd, 0.f, 0.f, 0.f);

	drawObjects(cmd, _renderables, camPos, camDir, sceneParameters, _pointLights);

	vkCmdEndRenderPass(cmd);

	drawTonemapping(cmd, swapchainImageIndex);

	vkEndCommandBuffer(cmd);
	

	//prepare the submission to the queue.
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,

		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame._presentSemaphore,

		.pWaitDstStageMask = &waitStage,

		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,

		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frame._renderSemaphore,
	};

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	vkQueueSubmit(_graphicsQueue, 1, &submit, frame._renderFence);

	// this will put the image we just rendered into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as it's necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,

		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame._renderSemaphore,

		.swapchainCount = 1,
		.pSwapchains = &_swapchain,

		.pImageIndices = &swapchainImageIndex
	};

	vkQueuePresentKHR(_graphicsQueue, &presentInfo);

	//increase the number of frames drawn
	_frameNumber++;
}

void Renderer::sortObjects(std::span<RenderObject> renderObjects) {
	std::sort(renderObjects.begin(), renderObjects.end(), [](RenderObject& a, RenderObject& b) {
		return a.material > b.material || (a.material == b.material && a.mesh > b.mesh);
		});
}

void Renderer::mapData(std::span<RenderObject> renderObjects, glm::mat4 view, glm::mat4 proj, GPUSceneData sceneParameters, std::span<PointLightObject> lights) {
	
	
	int frameIndex = _frameNumber % FRAME_OVERLAP;
	int frameOffset = (padUniformBufferSize(sizeof(GPUCameraData)) + padUniformBufferSize(sizeof(GPUSceneData))) * frameIndex;


	//fill a GPU camera data struct
	GPUCameraData camData{
		.view = view,
		.proj = proj,
		.viewproj = proj * view
	};

	//Copy camera and scene parameters into buffer
	{
		char* sceneData;
		vmaMapMemory(_allocator, _sceneParameterBuffer._allocation, (void**)&sceneData);

		sceneData += frameOffset;
		memcpy(sceneData, &camData, sizeof(GPUCameraData));

		sceneData += padUniformBufferSize(sizeof(GPUCameraData));
		memcpy(sceneData, &sceneParameters, sizeof(GPUSceneData));

		vmaUnmapMemory(_allocator, _sceneParameterBuffer._allocation);

	}

	// Map Object Data
	{
		GPUObjectData* objectData;
		vmaMapMemory(_allocator, getCurrentFrame().objectBuffer._allocation, (void**)&objectData);

		for (int i = 0; i < renderObjects.size(); i++) {
			objectData[i] = { .modelMatrix = renderObjects[i].transformMatrix };
		}

		vmaUnmapMemory(_allocator, getCurrentFrame().objectBuffer._allocation);

	}

	//Map Light Data
	{
		// Dir Light
		// TODO: proper implementation lol
		{
			int* data;
			vmaMapMemory(_allocator, getCurrentFrame().dirLightBuffer._allocation, (void**)&data);

			*data = 1;

			GPUDirLight* lightData = (GPUDirLight*)(data + 4);

			*lightData = {
				.lightPos = sceneParameters.camPos,
				.lightDir = sceneParameters.sunlightDirection,
				.lightSpaceMatrix = sceneParameters.lightSpaceMatrix,
				.lightColor = sceneParameters.sunlightColor,
				.ambientColor = sceneParameters.ambientColor,
				.shadowMapData = {
					0.f, 0.f, 0.5f
				}
			};

			vmaUnmapMemory(_allocator, getCurrentFrame().dirLightBuffer._allocation);
		}

		// Point light
		{
			int* data;
			vmaMapMemory(_allocator, getCurrentFrame().pointLightBuffer._allocation, (void**)&data);

			*data = lights.size();

			GPUPointLight* lightData = (GPUPointLight*)(data + 4);

			for (int i = 0; i < lights.size(); i++) {
				lightData[i] = generatePointLight(lights[i], (i * 6) + 1);
			}

			vmaUnmapMemory(_allocator, getCurrentFrame().pointLightBuffer._allocation);
		}
	}


}

// pos z = 4, neg z = 5
// pos y = 2, neg y = 3
// pos x = 0, neg x = 1
GPUPointLight Renderer::generatePointLight(PointLightObject light, uint32_t tile) {
	
	std::array<GPUShadowMapData, 6> sMP = {};

	for (int i = 0; i < 6; i++) {
		int tilePos = tile + i;
		int tileX = (tile + i) % 16;
		int tileY = (tile + i) / 16;

		
		sMP[i] = {(float)tileX/2.f, (float)tileY/2.f, 0.5f};
	}

	float near = 1.0f;
	float far = light.radius;
	glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.f, near, far);

	std::vector<glm::mat4> shadowTransforms;
	shadowTransforms.push_back(shadowProj * 
                 glm::lookAt(light.lightPos, light.lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * 
					glm::lookAt(light.lightPos, light.lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * 
					glm::lookAt(light.lightPos, light.lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
	shadowTransforms.push_back(shadowProj * 
					glm::lookAt(light.lightPos, light.lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
	shadowTransforms.push_back(shadowProj * 
					glm::lookAt(light.lightPos, light.lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * 
					glm::lookAt(light.lightPos, light.lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

	return {
		.lightPos = light.lightPos,
		.lightSpaceMatrix = {
			shadowTransforms[0], shadowTransforms[1], shadowTransforms[2], 
			shadowTransforms[3], shadowTransforms[4], shadowTransforms[5]
		},

		.lightColor = light.lightColor,
		.ambientColor = light.ambientColor,

		.radius = light.radius,
		.farPlane = light.radius,

		.shadowMapData = {
			sMP[0], sMP[1], sMP[2], sMP[3], sMP[4], sMP[5]
		}
	};


}

glm::mat4 Renderer::genCubeMapViewMatrix(uint8_t face, glm::vec3 lightPos) {
	float near = 1.0f;
	float far = 25.0f;
	glm::mat4 viewMatrix = glm::translate(lightPos);
	switch (face) {
		case 0: 		// POSITIVE_X
			viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;case 1:	// NEGATIVE_X
			viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;case 2:	// POSITIVE_Y
			viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;case 3:	// NEGATIVE_Y
			viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;case 4:	// POSITIVE_Z
			viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;case 5:	// NEGATIVE_Z
			viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		break;
	}

	glm::mat4 perspectiveMatrix = glm::perspective(glm::radians(90.0f), 1.f, near, far);

	return perspectiveMatrix * viewMatrix;

	//return viewMatrix;
}

glm::vec4 normalizePlane(glm::vec4 p) {
	return p / glm::length(glm::vec3(p));
}

inline uint32_t getGroupCount(uint32_t threadCount, uint32_t localSize) {
	return (threadCount + localSize - 1) / localSize;
}

void Renderer::genDepthPyramid(VkCommandBuffer cmd, uint32_t frameNumber) {

	// barrier the image into the shader read layout ready to generate depth buffer
	VkImageMemoryBarrier imageBarrier_toShaderRead = vkinit::imageBarrier(_mainFrameDepthImages[frameNumber]._image, VK_QUEUE_FAMILY_IGNORED,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_ASPECT_DEPTH_BIT);

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toShaderRead);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _depthPyramidPipeline);

	uint32_t depthPyramidWidth = previousPow2(_winSize.width);
	uint32_t depthPyramidHeight = previousPow2(_winSize.height);

	// hardcode due to not recreating depth pyramid images/views
	depthPyramidWidth = 1024;
	depthPyramidHeight = 512;
	uint32_t depthPyramidLevels = _depthPyramidViews.size();//getImageMipLevels(depthPyramidWidth, depthPyramidHeight);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _depthPyramidPipelineLayout,
		0, 1, &_depthPyramidSets[frameNumber], 0, nullptr);


	// TODO: look into computing multiple depths through one dispatch (for example, 64x64 area -> 1x1)
	for (uint32_t i = 0; i < depthPyramidLevels; i++) {

		uint32_t levelWidth = depthPyramidWidth >> i;
		uint32_t levelHeight = depthPyramidHeight >> i;
		if (levelHeight < 1) levelHeight = 1;
		if (levelWidth < 1) levelWidth = 1;

		GPUDepthReducePushConstants reduceData {
			i,
			{levelWidth, levelHeight}
		};

		vkCmdPushConstants(cmd, _depthPyramidPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(reduceData), &reduceData);

		vkCmdDispatch(cmd, getGroupCount(levelWidth, 32), getGroupCount(levelHeight, 32), 1);

		VkImageMemoryBarrier reduceBarrier = vkinit::imageBarrier(_depthPyramid._image, VK_QUEUE_FAMILY_IGNORED, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

		//barrier the image for next pass
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &reduceBarrier);
	}

	// Barrier image back to depth attachment to be used as part of main forward pass
	VkImageMemoryBarrier imageBarrier_toDepthAttachment = vkinit::imageBarrier(_mainFrameDepthImages[frameNumber]._image, VK_QUEUE_FAMILY_IGNORED,
		VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_DEPTH_BIT);

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toDepthAttachment);
		
}

void mat4Print(glm::mat4 matrix) {
	std::stringstream str;
	for (int i = 0; i < 4; i++) {
		str << "[";
		for (int j = 0; j < 4; j++)
			str << matrix[j][i] << ",";
		str << "]\n";
	}
	
	std::cout << str.str();
}

void Renderer::cullLightsPass(VkCommandBuffer cmd, std::span<PointLightObject> lights, glm::mat4 viewMatrix, glm::mat4 projMatrix, uint32_t swapchainIndex, float zNear, float zFar) {

	size_t lightCount = lights.size();

	size_t dispatchCount = ceil(lightCount / 64.f);

	int frameIndex = _frameNumber % FRAME_OVERLAP;
	int frameOffset = (padUniformBufferSize(sizeof(GPUCameraData)) + padUniformBufferSize(sizeof(GPUSceneData))) * frameIndex;
	uint32_t uniform_offset = frameOffset;
	uint32_t scene_offset = uniform_offset + padUniformBufferSize(sizeof(GPUCameraData));
	std::array<uint32_t, 2> offsets{ uniform_offset , scene_offset };

	vkCmdFillBuffer(cmd, getCurrentFrame().activeLightBuffer._buffer, 0, 4, 0);

	auto transferBarrier = vkinit::bufferBarrier(getCurrentFrame().activeLightBuffer._buffer, _graphicsQueueFamily, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &transferBarrier, 0, nullptr);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _lightCullPipeline);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _lightCullPipelineLayout,
		0, 1, &getCurrentFrame().globalDescriptor,
		offsets.size(), offsets.data());

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _lightCullPipelineLayout,
		1, 1, &getCurrentFrame().objectDescriptor,
		0, nullptr);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _lightCullPipelineLayout,
		2, 1, &_depthPyramidSets[swapchainIndex],
		0, nullptr);

	glm::mat4 projection = projMatrix;
	glm::mat4 projectionT = transpose(projection);
	
	glm::vec4 frustumX = normalizePlane(projectionT[3] + projectionT[0]); // x + w < 0
	glm::vec4 frustumY = normalizePlane(projectionT[3] + projectionT[1]); // y + w < 0

	GPULightCullPushConstants pushConstant{
		.viewMatrix = viewMatrix,
		.frustum = {
			frustumX.x,
			frustumX.z,
			frustumY.y,
			frustumY.z
		},
		.P00 = projection[0][0],
		.P11 = projection[1][1],
		.zNear = zNear,
		.zFar = zFar
	};

	vkCmdPushConstants(cmd, _lightCullPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
		0, sizeof(GPULightCullPushConstants), &pushConstant);

	vkCmdDispatch(cmd, dispatchCount, 1, 1);
	
	auto barrier = vkinit::bufferBarrier(getCurrentFrame().activeLightBuffer._buffer, _graphicsQueueFamily, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);


}

void Renderer::clusterLightsPass(VkCommandBuffer cmd, bool findClusters, glm::mat4 viewMatrix, glm::mat4 inverseMatrix, float zNear, float zFar) {
	auto preTransferBarrier = vkinit::bufferBarrier(getCurrentFrame().lightIndicesBuffer._buffer, _graphicsQueueFamily, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &preTransferBarrier, 0, nullptr);

	
	//vkCmdFillBuffer(cmd, getCurrentFrame().lightIndicesBuffer._buffer, 0, 4, 0);

	auto transferBarrier = vkinit::bufferBarrier(getCurrentFrame().lightIndicesBuffer._buffer, _graphicsQueueFamily, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &transferBarrier, 0, nullptr);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _clusterPipeline);

	int frameIndex = _frameNumber % FRAME_OVERLAP;
	int frameOffset = (padUniformBufferSize(sizeof(GPUCameraData)) + padUniformBufferSize(sizeof(GPUSceneData))) * frameIndex;
	uint32_t uniform_offset = frameOffset;
	uint32_t scene_offset = uniform_offset + padUniformBufferSize(sizeof(GPUCameraData));
	std::array<uint32_t, 2> offsets{ uniform_offset , scene_offset };


	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _clusterPipelineLayout,
		0, 1, &getCurrentFrame().globalDescriptor,
		offsets.size(), offsets.data());

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _clusterPipelineLayout,
		1, 1, &getCurrentFrame().objectDescriptor,
		0, nullptr);

	GPUClusterPushConstant constants {
		.findClusters = true,
		.zNear = zNear,
		.zFar = zFar,
		.inverseMatrix = inverseMatrix
	};

	vkCmdPushConstants(cmd, _clusterPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
		0, sizeof(constants), &constants);

	vkCmdDispatch(cmd, 1, 1, 24);

	auto barrier = vkinit::bufferBarrier(getCurrentFrame().lightIndicesBuffer._buffer, _graphicsQueueFamily, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

}

void Renderer::drawPrePass(VkCommandBuffer cmd, std::span<RenderObject> renderObjects, GPUSceneData sceneParameters, uint32_t frameIndex) {
	VkClearValue depthClear = {
		.depthStencil = {
			.depth = 0.f,
		}
	};

	VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = (float)_winSize.width,
		.height = (float)_winSize.height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	VkRect2D scissor = {
		.offset = {0,0},
		.extent = { (uint32_t)_winSize.width, (uint32_t)_winSize.height}
	};

	VkRenderPassBeginInfo rpInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,

		.renderPass = _prePassRenderPass,
		.framebuffer = _prePassFramebuffers[frameIndex],
		.renderArea{
			.offset = {0, 0},
			.extent = { (uint32_t)_winSize.width, (uint32_t)_winSize.height}
		},

		//connect clear values
		.clearValueCount = 1,
		.pClearValues = &depthClear
	};

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	vkCmdSetDepthBias(cmd, 0.f, 0.f, 0.f);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _prePassPipeline);

	frameIndex = _frameNumber % FRAME_OVERLAP;
	int frameOffset = (padUniformBufferSize(sizeof(GPUCameraData)) + padUniformBufferSize(sizeof(GPUSceneData))) * frameIndex;
	uint32_t uniform_offset = frameOffset;
	uint32_t scene_offset = uniform_offset + padUniformBufferSize(sizeof(GPUCameraData));
	std::array<uint32_t, 2> offsets{ uniform_offset , scene_offset };

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _prePassPipelineLayout, 0, 1, &getCurrentFrame().globalDescriptor, offsets.size(), offsets.data());

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _prePassPipelineLayout, 1, 1, &getCurrentFrame().objectDescriptor, 0, nullptr);

	auto draws = compactDraws(renderObjects);

	// TODO: generate drawCommands on the GPU
	{
		VkDrawIndexedIndirectCommand* drawCommands;
		vmaMapMemory(_allocator, getCurrentFrame().indirectBuffer._allocation, (void**)&drawCommands);

		for (uint32_t i = 0; i < draws.size(); i++) {
			auto& draw = draws[i];
			drawCommands[i] = {
				//.vertexCount = static_cast<uint32_t>(draw.mesh->_vertices.size()),
				.indexCount = static_cast<uint32_t>(draw.mesh->_indices.size()),
				.instanceCount = 1,
				.firstIndex = 0,
				.vertexOffset = 0,
				.firstInstance = i
			};

		}

		vmaUnmapMemory(_allocator, getCurrentFrame().indirectBuffer._allocation);
	}

	for (auto& draw : draws) {

		bindMesh(*draw.mesh, cmd);

		VkDeviceSize indirect_offset = draw.first * sizeof(VkDrawIndexedIndirectCommand);
		VkDeviceSize countOffset = draw.first * sizeof(uint32_t);

		uint32_t draw_stride = sizeof(VkDrawIndexedIndirectCommand);

		// TODO: move to using vkCmdDrawIndexedIndirectCount
		vkCmdDrawIndexedIndirect(cmd, getCurrentFrame().indirectBuffer._buffer, indirect_offset, 1, draw_stride);
		//vkCmdDrawIndexedIndirectCount(cmd, getCurrentFrame().indirectBuffer._buffer, indirect_offset, getCurrentFrame().indirectCount._buffer, countOffset, 1000000, draw_stride);
	}

	vkCmdEndRenderPass(cmd);
}

void Renderer::drawShadowPass(VkCommandBuffer cmd, std::span<RenderObject> renderObjects, GPUSceneData sceneParameters, std::span<PointLightObject> lights) {

	VkClearValue depthClear = {
		.depthStencil = {
			.depth = 1.f,
		}
	};

	VkRenderPassBeginInfo rpInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,

		.renderPass = _shadowRenderPass,
		.framebuffer = _shadowAtlasFrameBuffer,
		.renderArea{
			.offset = {0,0},
			.extent = {8192, 8192}
		},

		//connect clear values
		.clearValueCount = 1,
		.pClearValues = &depthClear
	};

	vkCmdBeginRenderPass(cmd,&rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = 8192,
		.height = 8192,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	VkRect2D scissor = {
		.offset = {0,0},
		.extent = {8192, 8192}
	};

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	vkCmdSetDepthBias(cmd, depthBiasConstantFactor, 0.f, depthBiasSlopeFactor);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipeline);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPipelineLayout, 0, 1, &getCurrentFrame().objectDescriptor, 0, nullptr);

	auto draws = compactDraws(renderObjects);

	// TODO: generate drawCommands on the GPU
	{
		VkDrawIndexedIndirectCommand* drawCommands;
		vmaMapMemory(_allocator, getCurrentFrame().indirectBuffer._allocation, (void**)&drawCommands);

		for (uint32_t i = 0; i < draws.size(); i++) {
			auto& draw = draws[i];
			drawCommands[i] = {
				//.vertexCount = static_cast<uint32_t>(draw.mesh->_vertices.size()),
				.indexCount = static_cast<uint32_t>(draw.mesh->_indices.size()),
				.instanceCount = 1,
				.firstIndex = 0,
				.vertexOffset = 0,
				.firstInstance = i
			};

		}

		vmaUnmapMemory(_allocator, getCurrentFrame().indirectBuffer._allocation);
	}

	uint32_t* drawCounts;
	vmaMapMemory(_allocator, getCurrentFrame().indirectCount._allocation, (void**)&drawCounts);

	for (int i = 0; i < draws.size(); i++) {

		drawCounts[i] = draws[i].count;
	}

	vmaUnmapMemory(_allocator, getCurrentFrame().indirectCount._allocation);

	drawShadow(cmd, draws, 0, 0, 0.f, 0.f, 0.5f);
	
	for (int i = 0; i < lights.size(); i++) {
		for (int j = 0; j < 6; j++) {
			float tileX = (((i * 6) + j + 1) % 16) / 2.0;
			float tileY = floor(((i * 6) + j + 1) / 16) / 2.0;
			drawShadow(cmd, draws, 1, (i * 6) + j, tileX, tileY, 0.5f);
		}
	}

	vkCmdEndRenderPass(cmd);
}

void Renderer::drawShadow(VkCommandBuffer cmd, std::span<IndirectBatch> draws, int type, int index, float x, float y, float size) {
	
	VkViewport viewport = {
		.x = x * 1024.f,
		.y = y * 1024.f,
		.width = size * 1024.f,
		.height = size * 1024.f,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	VkClearAttachment clearAttachment {
		.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		.clearValue = {
			.depthStencil = {
				.depth = 1.f,
			}
		}
	};

	VkClearRect clearRect {
		.rect = {
			.offset = { (int32_t)(x * 1024), (int32_t)(y * 1024)},
			.extent = { (uint32_t)(size * 1024), (uint32_t)(size * 1024)},
		},
		.layerCount = 1
	};

	vkCmdClearAttachments(cmd, 1, &clearAttachment, 1, &clearRect);

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	GPUShadowPushConstants shadowPushConstants {
		.lightType = type,
		.lightIndex = index
	};

	vkCmdPushConstants(cmd, _shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUShadowPushConstants), &shadowPushConstants);
	
	for (auto& draw : draws) {

		bindMesh(*draw.mesh, cmd);

		VkDeviceSize indirect_offset = draw.first * sizeof(VkDrawIndexedIndirectCommand);
		VkDeviceSize countOffset = draw.first * sizeof(uint32_t);

		uint32_t draw_stride = sizeof(VkDrawIndexedIndirectCommand);

		// TODO: move to vkCmdDrawIndexedIndirectCount
		vkCmdDrawIndexedIndirect(cmd, getCurrentFrame().indirectBuffer._buffer, indirect_offset, 1, draw_stride);
	}
}

void Renderer::drawObjects(VkCommandBuffer cmd, std::span<RenderObject> renderObjects, glm::vec3 camPos, glm::vec3 camDir, GPUSceneData sceneParameters, std::span<PointLightObject> lights) {

	int frameIndex = _frameNumber % FRAME_OVERLAP;
	int frameOffset = (padUniformBufferSize(sizeof(GPUCameraData)) + padUniformBufferSize(sizeof(GPUSceneData))) * frameIndex;

	auto draws = compactDraws(renderObjects);

	// TODO: generate drawCommands on the GPU
	{
		VkDrawIndexedIndirectCommand* drawCommands;
		vmaMapMemory(_allocator, getCurrentFrame().indirectBuffer._allocation, (void**)&drawCommands);

		for (uint32_t i = 0; i < draws.size(); i++) {
			auto& draw = draws[i];
			drawCommands[i] = {
				//.vertexCount = static_cast<uint32_t>(draw.mesh->_vertices.size()),
				.indexCount = static_cast<uint32_t>(draw.mesh->_indices.size()),
				.instanceCount = 1,
				.firstIndex = 0,
				.vertexOffset = 0,
				.firstInstance = i
			};

		}



		vmaUnmapMemory(_allocator, getCurrentFrame().indirectBuffer._allocation);
	}

	uint32_t* drawCounts;
	vmaMapMemory(_allocator, getCurrentFrame().indirectCount._allocation, (void**)&drawCounts);

	for (int i = 0; i < draws.size(); i++) {

		drawCounts[i] = draws[i].count;
	}

	vmaUnmapMemory(_allocator, getCurrentFrame().indirectCount._allocation);

	Material* lastMaterial = nullptr;

	for (auto& draw : draws) {

		if (draw.material != lastMaterial) bindMaterial(*draw.material, cmd, frameOffset);
		bindMesh(*draw.mesh, cmd);

		VkDeviceSize indirect_offset = draw.first * sizeof(VkDrawIndexedIndirectCommand);
		VkDeviceSize countOffset = draw.first * sizeof(uint32_t);

		uint32_t draw_stride = sizeof(VkDrawIndexedIndirectCommand);


		// TODO: move to vkCmdDrawIndexedIndirectCount
		vkCmdDrawIndexedIndirect(cmd, getCurrentFrame().indirectBuffer._buffer, indirect_offset, 1, draw_stride);

	}
}

void Renderer::drawTonemapping(VkCommandBuffer cmd, uint32_t imageIndex) {
	VkClearValue clearValue{
		.color = { { 0.0f, 0.0f, 0.0f, 1.0f } }
	};

	VkRenderPassBeginInfo rpInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,

		.renderPass = _tonemapRenderPass,
		.framebuffer = _tonemapFramebuffers[imageIndex],
		.renderArea{
			.offset = {0,0},
			.extent = {_actualWinSize.width, _actualWinSize.height}
		},

		//connect clear values
		.clearValueCount = 1,
		.pClearValues = &clearValue
	};

	VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = (float)_actualWinSize.width,
		.height = (float)_actualWinSize.height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	VkRect2D scissor = {
		.offset = {0,0},
		.extent = {_actualWinSize.width, _actualWinSize.height}
	};

	vkCmdBeginRenderPass(cmd,&rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	vkCmdSetDepthBias(cmd, 0.f, 0.f, 0.f);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _tonemapPipeline);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _tonemapPipelineLayout,
		0, 1, &_mainFrameImagesSets[imageIndex], 0, nullptr);
	
	vkCmdPushConstants(cmd, _tonemapPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &exposure);
	
	vkCmdDraw(cmd, 3, 1, 0, 0);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRenderPass(cmd);

}

std::vector<IndirectBatch> Renderer::compactDraws(std::span<RenderObject> objects) {

	std::vector<IndirectBatch> draws;

	IndirectBatch firstDraw;
	firstDraw.mesh = objects[0].mesh;
	firstDraw.material = objects[0].material;
	firstDraw.first = 0;
	firstDraw.count = 1;

	draws.push_back(firstDraw);

	for (int i = 1; i < objects.size(); i++) {
		//compare the mesh and material with the end of the vector of draws
		bool sameMesh = objects[i].mesh == draws.back().mesh;
		bool sameMaterial = objects[i].material == draws.back().material;

		if (sameMesh && sameMaterial)
		{
			//all matches, add count
			draws.back().count++;
		}
		else
		{
			//add new draw
			IndirectBatch newDraw;
			newDraw.mesh = objects[i].mesh;
			newDraw.material = objects[i].material;
			newDraw.first = i;
			newDraw.count = 1;

			draws.push_back(newDraw);
		}
	}
	return draws;
}

void Renderer::bindMaterial(const Material& material, VkCommandBuffer cmd, uint32_t frameOffset) {
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);

	uint32_t uniform_offset = frameOffset;
	uint32_t scene_offset = uniform_offset + padUniformBufferSize(sizeof(GPUCameraData));
	std::array<uint32_t, 2> offsets{ uniform_offset , scene_offset };

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipelineLayout, 0, 1, &getCurrentFrame().globalDescriptor, offsets.size(), offsets.data());

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipelineLayout, 1, 1, &getCurrentFrame().objectDescriptor, 0, nullptr);

	if (material.textureSet != VK_NULL_HANDLE) {
		//texture descriptor
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipelineLayout, 2, 1, &material.textureSet, 0, nullptr);

	}

	if (material.specularSet != nullptr) {
		//texture descriptor
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipelineLayout, 3, 1, &material.specularSet, 0, nullptr);

	}
}

void Renderer::bindMesh(const Mesh& mesh, VkCommandBuffer cmd) {
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &mesh._vertexBuffer._buffer, &offset);
	vkCmdBindIndexBuffer(cmd, mesh._indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
}

FrameData& Renderer::getCurrentFrame() {
	return _frames[_frameNumber % FRAME_OVERLAP];
}