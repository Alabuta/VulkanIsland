#pragma once

#include "main.h"
#include "device.h"



void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                  VkBuffer &buffer, VkDeviceMemory &deviceMemory, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

void CreateImage(VkPhysicalDevice physicalDevice, VkDevice device,
                 VkImage &image, VkDeviceMemory &deviceMemory, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                 VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);


void CreateUniformBuffer(VulkanDevice *vulkanDevice, VkBuffer &uboBuffer, VkDeviceMemory &uboBufferMemory, std::size_t size);