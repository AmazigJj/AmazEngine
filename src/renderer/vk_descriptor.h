#pragma once

#include <stdint.h>
#include <vector>
#include <optional>

#include <vulkan/vulkan.h>

#include "util/ShaderStages.h"
#include "util/BindingType.h"


namespace amaz::eng {

	struct BindingInfo {
		uint32_t binding;
		uint32_t count;
		BindingType type;
		ShaderStages stage;
		std::optional<VkDescriptorBufferInfo> buffer_info;
		VkDescriptorBufferInfo* ptr_buffer_info;
		std::optional<VkDescriptorImageInfo> image_info;
		VkDescriptorImageInfo* ptr_image_info;
		VkSampler* immutableSampler;
	};

	// TODO: make non-vulkan specific
	class DescriptorBuilder {
	public:
		static DescriptorBuilder init() {return DescriptorBuilder{};}
		DescriptorBuilder& add_buffer(uint32_t binding, uint32_t count, BindingType type, ShaderStages stage, std::optional<VkDescriptorBufferInfo> buffer_info);
		DescriptorBuilder& add_buffer(uint32_t binding, uint32_t count, BindingType type, ShaderStages stage, VkDescriptorBufferInfo* buffer_info = nullptr);
		DescriptorBuilder& add_image(uint32_t binding, uint32_t count, BindingType type, ShaderStages stage, std::optional<VkDescriptorImageInfo> image_info, VkSampler* immutableSampler = nullptr);
		DescriptorBuilder& add_image(uint32_t binding, uint32_t count, BindingType type, ShaderStages stage, VkDescriptorImageInfo* image_info = nullptr, VkSampler* immutableSampler = nullptr);
		
		VkDescriptorSetLayout build_layout(VkDevice device);
		VkDescriptorSet build_set(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout);

	private:
		std::vector<BindingInfo> bindings;

	};

}