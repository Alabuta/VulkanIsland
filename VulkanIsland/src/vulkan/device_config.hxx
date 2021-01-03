#pragma once

#include <array>
#include <tuple>

#include "vulkan/instance.hxx"

namespace vulkan
{
    inline auto constexpr device_extensions = std::array{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,

        /*VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,*/

        //VK_KHR_INLINE_UNIFORM_BLOCK_EXTENSION_NAME

    #if OBSOLETE
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,

        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
        VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,

        VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,

        VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME
    #endif
    };

    inline VkPhysicalDeviceFeatures constexpr device_features{
        VK_FALSE, // robustBufferAccess,
        VK_FALSE, // fullDrawIndexUint32,
        VK_TRUE,  // imageCubeArray,
        VK_FALSE, // independentBlend,
        VK_TRUE,  // geometryShader,
        VK_TRUE,  // tessellationShader,
        VK_FALSE, // sampleRateShading,
        VK_FALSE, // dualSrcBlend,
        VK_FALSE, // logicOp,
        VK_TRUE,  // multiDrawIndirect,
        VK_TRUE,  // drawIndirectFirstInstance,
        VK_TRUE,  // depthClamp,
        VK_FALSE, // depthBiasClamp,
        VK_TRUE,  // fillModeNonSolid,
        VK_FALSE, // depthBounds,
        VK_FALSE, // wideLines,
        VK_FALSE, // largePoints,
        VK_FALSE, // alphaToOne,
        VK_FALSE, // multiViewport,
        VK_TRUE,  // samplerAnisotropy,
        VK_FALSE, // textureCompressionETC2,
        VK_FALSE, // textureCompressionASTC_LDR,
        VK_FALSE, // textureCompressionBC,
        VK_FALSE, // occlusionQueryPrecise,
        VK_FALSE, // pipelineStatisticsQuery,
        VK_FALSE, // vertexPipelineStoresAndAtomics,
        VK_FALSE, // fragmentStoresAndAtomics,
        VK_FALSE, // shaderTessellationAndGeometryPointSize,
        VK_FALSE, // shaderImageGatherExtended,
        VK_FALSE, // shaderStorageImageExtendedFormats,
        VK_FALSE, // shaderStorageImageMultisample,
        VK_FALSE, // shaderStorageImageReadWithoutFormat,
        VK_FALSE, // shaderStorageImageWriteWithoutFormat,
        VK_TRUE,  // shaderUniformBufferArrayDynamicIndexing,
        VK_TRUE,  // shaderSampledImageArrayDynamicIndexing,
        VK_TRUE,  // shaderStorageBufferArrayDynamicIndexing,
        VK_TRUE,  // shaderStorageImageArrayDynamicIndexing,
        VK_FALSE, // shaderClipDistance,
        VK_FALSE, // shaderCullDistance,
        VK_FALSE, // shaderFloat64,
        VK_FALSE, // shaderInt64,
        VK_TRUE,  // shaderInt16,
        VK_FALSE, // shaderResourceResidency,
        VK_FALSE, // shaderResourceMinLod,
        VK_TRUE, // sparseBinding,
        VK_FALSE, // sparseResidencyBuffer,
        VK_FALSE, // sparseResidencyImage2D,
        VK_FALSE, // sparseResidencyImage3D,
        VK_FALSE, // sparseResidency2Samples,
        VK_FALSE, // sparseResidency4Samples,
        VK_FALSE, // sparseResidency8Samples,
        VK_FALSE, // sparseResidency16Samples,
        VK_FALSE, // sparseResidencyAliased,
        VK_FALSE, // variableMultisampleRate,
        VK_FALSE, // inheritedQueries
    };

    inline auto constexpr device_extended_features = std::tuple{
        VkPhysicalDevice8BitStorageFeaturesKHR{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR,
            nullptr,
            VK_TRUE, VK_TRUE, VK_TRUE
        },
        VkPhysicalDevice16BitStorageFeatures{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES,
            nullptr,
            VK_TRUE, VK_TRUE, VK_TRUE, VK_FALSE
        },
        VkPhysicalDeviceFloat16Int8FeaturesKHR{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR,
            nullptr,
            VK_FALSE, VK_TRUE
        }
    };
}
