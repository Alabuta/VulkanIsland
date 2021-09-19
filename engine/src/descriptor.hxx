#pragma once

#include <optional>
#include <iostream>
#include <memory>
#include <span>

#include <fmt/format.h>

#include "main.hxx"
#include "utility/exceptions.hxx"
#include "vulkan/device.hxx"


std::optional<VkDescriptorPool> create_descriptor_pool(vulkan::device const &device);

std::vector<VkDescriptorSet>
create_descriptor_sets(vulkan::device const &device, VkDescriptorPool descriptorPool, std::span<VkDescriptorSetLayout const> const descriptorSetLayouts);

std::optional<VkDescriptorSetLayout>
create_descriptor_set_layout2(vulkan::device const &device, std::span<const VkDescriptorSetLayoutBinding> const layout_bindings);

std::optional<VkDescriptorSetLayout> create_view_resources_descriptor_set_layout(vulkan::device const &device);
std::optional<VkDescriptorSetLayout> create_object_resources_descriptor_set_layout(vulkan::device const &device);
