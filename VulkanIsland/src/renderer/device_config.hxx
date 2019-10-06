#pragma once

#include "instance.hxx"

VkPhysicalDeviceFeatures constexpr kDEVICE_FEATURES{
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
    VkBool32(VK_FALSE), // shaderInt16,
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

constexpr bool ComparePhysicalDeviceFeatures(VkPhysicalDeviceFeatures const &rhs)
{
    auto total = VkBool32(VK_TRUE);

#define COMPARE_FIELDS_BY_LHS(field)                (kDEVICE_FEATURES.field == (kDEVICE_FEATURES.field * rhs.field))

    total *= COMPARE_FIELDS_BY_LHS(robustBufferAccess);
    total *= COMPARE_FIELDS_BY_LHS(fullDrawIndexUint32);
    total *= COMPARE_FIELDS_BY_LHS(imageCubeArray);
    total *= COMPARE_FIELDS_BY_LHS(independentBlend);
    total *= COMPARE_FIELDS_BY_LHS(geometryShader);
    total *= COMPARE_FIELDS_BY_LHS(tessellationShader);
    total *= COMPARE_FIELDS_BY_LHS(sampleRateShading);
    total *= COMPARE_FIELDS_BY_LHS(dualSrcBlend);
    total *= COMPARE_FIELDS_BY_LHS(logicOp);
    total *= COMPARE_FIELDS_BY_LHS(multiDrawIndirect);
    total *= COMPARE_FIELDS_BY_LHS(drawIndirectFirstInstance);
    total *= COMPARE_FIELDS_BY_LHS(depthClamp);
    total *= COMPARE_FIELDS_BY_LHS(depthBiasClamp);
    total *= COMPARE_FIELDS_BY_LHS(fillModeNonSolid);
    total *= COMPARE_FIELDS_BY_LHS(depthBounds);
    total *= COMPARE_FIELDS_BY_LHS(wideLines);
    total *= COMPARE_FIELDS_BY_LHS(largePoints);
    total *= COMPARE_FIELDS_BY_LHS(alphaToOne);
    total *= COMPARE_FIELDS_BY_LHS(multiViewport);
    total *= COMPARE_FIELDS_BY_LHS(samplerAnisotropy);
    total *= COMPARE_FIELDS_BY_LHS(textureCompressionETC2);
    total *= COMPARE_FIELDS_BY_LHS(textureCompressionASTC_LDR);
    total *= COMPARE_FIELDS_BY_LHS(textureCompressionBC);
    total *= COMPARE_FIELDS_BY_LHS(occlusionQueryPrecise);
    total *= COMPARE_FIELDS_BY_LHS(pipelineStatisticsQuery);
    total *= COMPARE_FIELDS_BY_LHS(vertexPipelineStoresAndAtomics);
    total *= COMPARE_FIELDS_BY_LHS(fragmentStoresAndAtomics);
    total *= COMPARE_FIELDS_BY_LHS(shaderTessellationAndGeometryPointSize);
    total *= COMPARE_FIELDS_BY_LHS(shaderImageGatherExtended);
    total *= COMPARE_FIELDS_BY_LHS(shaderStorageImageExtendedFormats);
    total *= COMPARE_FIELDS_BY_LHS(shaderStorageImageMultisample);
    total *= COMPARE_FIELDS_BY_LHS(shaderStorageImageReadWithoutFormat);
    total *= COMPARE_FIELDS_BY_LHS(shaderStorageImageWriteWithoutFormat);
    total *= COMPARE_FIELDS_BY_LHS(shaderUniformBufferArrayDynamicIndexing);
    total *= COMPARE_FIELDS_BY_LHS(shaderSampledImageArrayDynamicIndexing);
    total *= COMPARE_FIELDS_BY_LHS(shaderStorageBufferArrayDynamicIndexing);
    total *= COMPARE_FIELDS_BY_LHS(shaderStorageImageArrayDynamicIndexing);
    total *= COMPARE_FIELDS_BY_LHS(shaderClipDistance);
    total *= COMPARE_FIELDS_BY_LHS(shaderCullDistance);
    total *= COMPARE_FIELDS_BY_LHS(shaderFloat64);
    total *= COMPARE_FIELDS_BY_LHS(shaderInt64);
    total *= COMPARE_FIELDS_BY_LHS(shaderInt16);
    total *= COMPARE_FIELDS_BY_LHS(shaderResourceResidency);
    total *= COMPARE_FIELDS_BY_LHS(shaderResourceMinLod);
    total *= COMPARE_FIELDS_BY_LHS(sparseBinding);
    total *= COMPARE_FIELDS_BY_LHS(sparseResidencyBuffer);
    total *= COMPARE_FIELDS_BY_LHS(sparseResidencyImage2D);
    total *= COMPARE_FIELDS_BY_LHS(sparseResidencyImage3D);
    total *= COMPARE_FIELDS_BY_LHS(sparseResidency2Samples);
    total *= COMPARE_FIELDS_BY_LHS(sparseResidency4Samples);
    total *= COMPARE_FIELDS_BY_LHS(sparseResidency8Samples);
    total *= COMPARE_FIELDS_BY_LHS(sparseResidency16Samples);
    total *= COMPARE_FIELDS_BY_LHS(sparseResidencyAliased);
    total *= COMPARE_FIELDS_BY_LHS(variableMultisampleRate);
    total *= COMPARE_FIELDS_BY_LHS(inheritedQueries);

#undef COMPARE_BY_FIELDS

    return total != VkBool32(VK_FALSE);
}