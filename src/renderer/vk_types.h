#pragma once

#include <volk.h>
#define VK_NO_PROTOTYPES
#include <vk_mem_alloc.h>

struct AllocatedBuffer {
	VkBuffer _buffer;
	VmaAllocation _allocation;
};

struct AllocatedImage {
	VkImage _image;
	VmaAllocation _allocation;
};




