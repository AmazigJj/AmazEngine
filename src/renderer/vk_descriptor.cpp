#include "vk_descriptor.h"
#include <vulkan/vulkan_core.h>
#include <iostream>

namespace amaz::eng {

	DescriptorBuilder& DescriptorBuilder::add_buffer(uint32_t binding, uint32_t count, BindingType type, ShaderStages stage, std::optional<VkDescriptorBufferInfo> buffer_info) {

		//std::cout << std::format("BINDING TYPE: {}\n", (int)type);
		BindingInfo bindInfo {
			.binding = binding,
			.count = count,
			.type = type,
			.stage = stage,
			.buffer_info = buffer_info,
			.ptr_buffer_info = nullptr,
			.image_info = std::nullopt,
			.ptr_image_info = nullptr,
			.immutableSampler = nullptr
		};
		bindings.push_back(bindInfo);
		return *this;
	}

	DescriptorBuilder& DescriptorBuilder::add_buffer(uint32_t binding, uint32_t count, BindingType type, ShaderStages stage, VkDescriptorBufferInfo* buffer_info) {

		//std::cout << std::format("BINDING TYPE: {}\n", (int)type);
		BindingInfo bindInfo {
			.binding = binding,
			.count = count,
			.type = type,
			.stage = stage,
			.buffer_info = std::nullopt,
			.ptr_buffer_info = buffer_info,
			.image_info = std::nullopt,
			.ptr_image_info = nullptr,
			.immutableSampler = nullptr
		};
		bindings.push_back(bindInfo);
		return *this;
	}

	DescriptorBuilder& DescriptorBuilder::add_image(uint32_t binding, uint32_t count, BindingType type, ShaderStages stage, std::optional<VkDescriptorImageInfo> image_info, VkSampler* immutableSampler) {
		BindingInfo bindInfo {
			.binding = binding,
			.count = count,
			.type = type,
			.stage = stage,
			.buffer_info = std::nullopt,
			.ptr_buffer_info = nullptr,
			.image_info = image_info,
			.ptr_image_info = nullptr,
			.immutableSampler = immutableSampler
		};
		bindings.push_back(bindInfo);
		return *this;
	}

	DescriptorBuilder& DescriptorBuilder::add_image(uint32_t binding, uint32_t count, BindingType type, ShaderStages stage, VkDescriptorImageInfo* image_info, VkSampler* immutableSampler) {
		BindingInfo bindInfo {
			.binding = binding,
			.count = count,
			.type = type,
			.stage = stage,
			.buffer_info = std::nullopt,
			.ptr_buffer_info = nullptr,
			.image_info = std::nullopt,
			.ptr_image_info = image_info,
			.immutableSampler = immutableSampler
		};
		bindings.push_back(bindInfo);
		return *this;
	}

	VkDescriptorSetLayout DescriptorBuilder::build_layout(VkDevice device) {
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		for (auto& bind : bindings) {
			//std::cout << std::format("Binding: {}\n", bind.binding);
			VkDescriptorSetLayoutBinding newBind = {
				.binding = bind.binding,
				.descriptorType = (VkDescriptorType)bind.type,
				.descriptorCount = bind.count,
				.stageFlags = (VkShaderStageFlags)bind.stage,
				.pImmutableSamplers = bind.immutableSampler
			};
			layoutBindings.push_back(newBind);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,

			.bindingCount = (uint32_t)layoutBindings.size(),
			.pBindings = layoutBindings.data(),
		};

		VkDescriptorSetLayout layout;
		vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);
		return layout;
	}

	VkDescriptorSet DescriptorBuilder::build_set(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout) {

		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,

			.descriptorPool = pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &layout
		};

		VkDescriptorSet set;
		vkAllocateDescriptorSets(device, &allocInfo, &set);

		std::vector<VkWriteDescriptorSet> setWrites;

		for (auto& bind : bindings) {
			VkWriteDescriptorSet setWrite {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,

				.dstSet = set,
				.dstBinding = bind.binding,
				.descriptorCount = bind.count,
				.descriptorType = (VkDescriptorType)bind.type,
				.pImageInfo = bind.image_info ? &bind.image_info.value() : bind.ptr_image_info,
				.pBufferInfo = bind.buffer_info ? &bind.buffer_info.value() : bind.ptr_buffer_info
			};
			setWrites.push_back(setWrite);
		}

		vkUpdateDescriptorSets(device, setWrites.size(), setWrites.data(), 0, nullptr);		

		return set;
	}

}