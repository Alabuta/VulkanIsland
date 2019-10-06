#pragma once

#include <array>

#include "vulkan_instance.hxx"

namespace vulkan
{
    auto constexpr device_extensions = std::array{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,

        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,

        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,
        /*VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,*/
        VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,

        VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME
        //VK_KHR_INLINE_UNIFORM_BLOCK_EXTENSION_NAME
    };

    VkPhysicalDeviceFeatures constexpr device_features{
        VkBool32(VK_FALSE), // robustBufferAccess,
        VkBool32(VK_FALSE), // fullDrawIndexUint32,
        VkBool32(VK_FALSE), // imageCubeArray,
        VkBool32(VK_FALSE), // independentBlend,
        VkBool32(VK_TRUE),  // geometryShader,
        VkBool32(VK_TRUE),  // tessellationShader,
        VkBool32(VK_FALSE), // sampleRateShading,
        VkBool32(VK_FALSE), // dualSrcBlend,
        VkBool32(VK_FALSE), // logicOp,
        VkBool32(VK_TRUE),  // multiDrawIndirect,
        VkBool32(VK_FALSE), // drawIndirectFirstInstance,
        VkBool32(VK_TRUE),  // depthClamp,
        VkBool32(VK_FALSE), // depthBiasClamp,
        VkBool32(VK_TRUE),  // fillModeNonSolid,
        VkBool32(VK_FALSE), // depthBounds,
        VkBool32(VK_FALSE), // wideLines,
        VkBool32(VK_FALSE), // largePoints,
        VkBool32(VK_FALSE), // alphaToOne,
        VkBool32(VK_FALSE), // multiViewport,
        VkBool32(VK_TRUE),  // samplerAnisotropy,
        VkBool32(VK_FALSE), // textureCompressionETC2,
        VkBool32(VK_FALSE), // textureCompressionASTC_LDR,
        VkBool32(VK_FALSE), // textureCompressionBC,
        VkBool32(VK_FALSE), // occlusionQueryPrecise,
        VkBool32(VK_FALSE), // pipelineStatisticsQuery,
        VkBool32(VK_FALSE), // vertexPipelineStoresAndAtomics,
        VkBool32(VK_FALSE), // fragmentStoresAndAtomics,
        VkBool32(VK_FALSE), // shaderTessellationAndGeometryPointSize,
        VkBool32(VK_FALSE), // shaderImageGatherExtended,
        VkBool32(VK_FALSE), // shaderStorageImageExtendedFormats,
        VkBool32(VK_FALSE), // shaderStorageImageMultisample,
        VkBool32(VK_FALSE), // shaderStorageImageReadWithoutFormat,
        VkBool32(VK_FALSE), // shaderStorageImageWriteWithoutFormat,
        VkBool32(VK_TRUE),  // shaderUniformBufferArrayDynamicIndexing,
        VkBool32(VK_TRUE),  // shaderSampledImageArrayDynamicIndexing,
        VkBool32(VK_TRUE),  // shaderStorageBufferArrayDynamicIndexing,
        VkBool32(VK_TRUE),  // shaderStorageImageArrayDynamicIndexing,
        VkBool32(VK_FALSE), // shaderClipDistance,
        VkBool32(VK_FALSE), // shaderCullDistance,
        VkBool32(VK_FALSE), // shaderFloat64,
        VkBool32(VK_FALSE), // shaderInt64,
        VkBool32(VK_TRUE),  // shaderInt16,
        VkBool32(VK_FALSE), // shaderResourceResidency,
        VkBool32(VK_FALSE), // shaderResourceMinLod,
        VkBool32(VK_FALSE), // sparseBinding,
        VkBool32(VK_FALSE), // sparseResidencyBuffer,
        VkBool32(VK_FALSE), // sparseResidencyImage2D,
        VkBool32(VK_FALSE), // sparseResidencyImage3D,
        VkBool32(VK_FALSE), // sparseResidency2Samples,
        VkBool32(VK_FALSE), // sparseResidency4Samples,
        VkBool32(VK_FALSE), // sparseResidency8Samples,
        VkBool32(VK_FALSE), // sparseResidency16Samples,
        VkBool32(VK_FALSE), // sparseResidencyAliased,
        VkBool32(VK_FALSE), // variableMultisampleRate,
        VkBool32(VK_FALSE), // inheritedQueries
    };
}
