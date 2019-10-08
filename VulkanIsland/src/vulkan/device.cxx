#include <algorithm>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "utility/mpl.hxx"

#define USE_DEBUG_MARKERS 0

#include "vulkan_config.hxx"
#include "device_config.hxx"
#include "device.hxx"
#include "renderer/queue_builder.hxx"
#include "renderer/swapchain.hxx"


namespace
{
    struct required_device_queues final {
        std::vector<GraphicsQueue> &graphics_queues_;
        std::vector<ComputeQueue> &compute_queues_;
        std::vector<TransferQueue> &transfer_queues_;
        std::vector<PresentationQueue> &presentation_queues_;
    };

    template<class T, std::size_t I = 0>
    constexpr bool check_all_queues_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
    {
        using Q = std::variant_alternative_t<I, T>;

        if (!QueueHelper::IsSupportedByDevice<Q>(physical_device, surface))
            return false;

        if constexpr (I + 1 < std::variant_size_v<T>)
            return check_all_queues_support<T, I + 1>(physical_device, surface);

        return false;
    }

    template<bool check_on_duplicates = false>
    [[nodiscard]] bool check_required_device_extensions(VkPhysicalDevice physical_device, std::vector<std::string_view> &&extensions)
    {
        std::vector<VkExtensionProperties> required_extensions;

        auto constexpr extensions_compare = [] (auto &&lhs, auto &&rhs)
        {
            return std::lexicographical_compare(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName),
                                                std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
        };

        std::transform(std::cbegin(extensions), std::cend(extensions), std::back_inserter(required_extensions), [] (auto &&name)
        {
            VkExtensionProperties prop{};
            std::uninitialized_copy_n(std::begin(name), std::size(name), prop.extensionName);

            return prop;
        });

        std::sort(std::begin(required_extensions), std::end(required_extensions), extensions_compare);

        if constexpr (check_on_duplicates) {
            auto it = std::unique(std::begin(required_extensions), std::end(required_extensions), [] (auto &&lhs, auto &&rhs)
            {
                return std::equal(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName),
                                  std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
            });

            required_extensions.erase(it, std::end(required_extensions));
        }

        std::uint32_t extensions_count = 0;

        if (auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, nullptr); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device extensions count: {0:#x}\n"s, result));

        std::vector<VkExtensionProperties> supported_extensions(extensions_count);

        if (auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, std::data(supported_extensions)); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve device extensions: {0:#x}\n"s, result));

        std::sort(std::begin(supported_extensions), std::end(supported_extensions), extensions_compare);

        return std::includes(std::cbegin(supported_extensions), std::cend(supported_extensions),
                             std::cbegin(required_extensions), std::cend(required_extensions), extensions_compare);
    }

    bool constexpr compare_physical_device_features(VkPhysicalDeviceFeatures const &lhs, VkPhysicalDeviceFeatures const &rhs)
    {
        auto total = VkBool32(VK_TRUE);

    #define COMPARE_FIELDS_BY_LHS(field)                (lhs.field == (lhs.field * rhs.field))

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

    [[nodiscard]] VkPhysicalDevice
    pick_physical_device(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extensions, required_device_queues &required_queues)
    {
        std::uint32_t devices_count = 0;

        if (auto result = vkEnumeratePhysicalDevices(instance, &devices_count, nullptr); result != VK_SUCCESS || devices_count == 0)
            throw std::runtime_error("failed to find physical device with Vulkan API support: "s + std::to_string(result));

        std::vector<VkPhysicalDevice> devices(devices_count);

        if (auto result = vkEnumeratePhysicalDevices(instance, &devices_count, std::data(devices)); result != VK_SUCCESS)
            throw std::runtime_error("failed to retrieve physical devices: "s + std::to_string(result));

        auto const application_info = vulkan_config::application_info;

        auto const device_features = vulkan::device_features;

        // Matching by supported properties, features and extensions.
        auto it_end = std::remove_if(std::begin(devices), std::end(devices), [&extensions, &application_info, &device_features] (auto &&device)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            if (properties.apiVersion < application_info.apiVersion)
                return true;

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);

            if (!compare_physical_device_features(device_features, features))
                return true;

            return !check_required_device_extensions(device, std::move(extensions));
        });

        devices.erase(it_end, std::end(devices));

        // Removing unsuitable devices. Matching by required compute, graphics, transfer and presentation queues.
        it_end = std::remove_if(std::begin(devices), std::end(devices), [&] (auto &&device)
        {
            auto check_queue_pool_support = [device, surface] (auto &&queue_pool)
            {
                if (queue_pool.empty())
                    return true;

                return QueueHelper::IsSupportedByDevice<typename std::remove_cvref_t<decltype(queue_pool)>::value_type>(device, surface);
            };

            if (!check_queue_pool_support(required_queues.compute_queues_))
                return true;

            if (!check_queue_pool_support(required_queues.graphics_queues_))
                return true;

            if (!check_queue_pool_support(required_queues.presentation_queues_))
                return true;

            if (!check_queue_pool_support(required_queues.transfer_queues_))
                return true;

            return false;
        });

        devices.erase(it_end, std::end(devices));

        // Matching by the swap chain properties support.
        it_end = std::remove_if(std::begin(devices), std::end(devices), [surface] (auto &&device)
        {
            auto const details = QuerySwapChainSupportDetails(device, surface);

            return details.formats.empty() || details.presentModes.empty();
        });

        devices.erase(it_end, std::end(devices));

        if (devices.empty())
            throw std::runtime_error("failed to pick physical device"s);

        auto constexpr device_types_priority = std::array{
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
            VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_CPU
        };

        // Sorting by device type.
        for (auto device_type : device_types_priority) {
            auto id_next_type = std::stable_partition(std::begin(devices), std::end(devices), [device_type] (auto &&device)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);

                return properties.deviceType == device_type;
            });

            if (id_next_type != std::end(devices))
                break;
        }

        return devices.front();
    }

    [[nodiscard]] std::vector<VkDeviceQueueCreateInfo>
    get_queue_infos(VkPhysicalDevice physical_handle, VkSurfaceKHR surface, std::vector<char const *> &&extensions,
                    required_device_queues &required_queues, std::vector<std::vector<float>> &priorities)
    {
        QueueHelper queue_helper;

        for (auto &&queue : required_queues.compute_queues_)
            queue = queue_helper.Find<std::remove_cvref_t<decltype(queue)>>(physical_handle, surface);

        for (auto &&queue : required_queues.graphics_queues_)
            queue = queue_helper.Find<std::remove_cvref_t<decltype(queue)>>(physical_handle, surface);

        for (auto &&queue : required_queues.transfer_queues_)
            queue = queue_helper.Find<std::remove_cvref_t<decltype(queue)>>(physical_handle, surface);

        for (auto &&queue : required_queues.presentation_queues_)
            queue = queue_helper.Find<std::remove_cvref_t<decltype(queue)>>(physical_handle, surface);

        std::vector<VkDeviceQueueCreateInfo> queue_infos;

        for (auto [family, count] : queue_helper.GetRequestedFamilies()) {
            priorities.emplace_back(count, 1.f);

            VkDeviceQueueCreateInfo queue_info{
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                nullptr, 0,
                family, count,
                std::data(priorities.back())
            };

            queue_infos.push_back(std::move(queue_info));
        }

        return queue_infos;
    }

    void get_device_queues(VkDevice handle, required_device_queues &required_queues)
    {
        for (auto &&queue : required_queues.compute_queues_)
            vkGetDeviceQueue(handle, queue.family_, queue.index_, &queue.handle_);

        for (auto &&queue : required_queues.graphics_queues_)
            vkGetDeviceQueue(handle, queue.family_, queue.index_, &queue.handle_);

        for (auto &&queue : required_queues.transfer_queues_)
            vkGetDeviceQueue(handle, queue.family_, queue.index_, &queue.handle_);

        for (auto &&queue : required_queues.presentation_queues_)
            vkGetDeviceQueue(handle, queue.family_, queue.index_, &queue.handle_);
    }

    [[nodiscard]] vulkan::device_limits get_device_limits(VkPhysicalDevice physical_handle)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_handle, &properties);

        auto &&limits = properties.limits;

        return vulkan::device_limits{
            limits.maxImageDimension1D,
            limits.maxImageDimension2D,
            limits.maxImageDimension3D,
            limits.maxImageDimensionCube,
            limits.maxImageArrayLayers,
            limits.maxTexelBufferElements,
            limits.maxUniformBufferRange,
            limits.maxStorageBufferRange,
            limits.maxPushConstantsSize,
            limits.maxMemoryAllocationCount,
            limits.maxSamplerAllocationCount,
            limits.bufferImageGranularity,
            limits.sparseAddressSpaceSize,
            limits.maxBoundDescriptorSets,
            limits.maxPerStageDescriptorSamplers,
            limits.maxPerStageDescriptorUniformBuffers,
            limits.maxPerStageDescriptorStorageBuffers,
            limits.maxPerStageDescriptorSampledImages,
            limits.maxPerStageDescriptorStorageImages,
            limits.maxPerStageDescriptorInputAttachments,
            limits.maxPerStageResources,
            limits.maxDescriptorSetSamplers,
            limits.maxDescriptorSetUniformBuffers,
            limits.maxDescriptorSetUniformBuffersDynamic,
            limits.maxDescriptorSetStorageBuffers,
            limits.maxDescriptorSetStorageBuffersDynamic,
            limits.maxDescriptorSetSampledImages,
            limits.maxDescriptorSetStorageImages,
            limits.maxDescriptorSetInputAttachments,
            limits.maxVertexInputAttributes,
            limits.maxVertexInputBindings,
            limits.maxVertexInputAttributeOffset,
            limits.maxVertexInputBindingStride,
            limits.maxVertexOutputComponents,
            limits.maxTessellationGenerationLevel,
            limits.maxTessellationPatchSize,
            limits.maxTessellationControlPerVertexInputComponents,
            limits.maxTessellationControlPerVertexOutputComponents,
            limits.maxTessellationControlPerPatchOutputComponents,
            limits.maxTessellationControlTotalOutputComponents,
            limits.maxTessellationEvaluationInputComponents,
            limits.maxTessellationEvaluationOutputComponents,
            limits.maxGeometryShaderInvocations,
            limits.maxGeometryInputComponents,
            limits.maxGeometryOutputComponents,
            limits.maxGeometryOutputVertices,
            limits.maxGeometryTotalOutputComponents,
            limits.maxFragmentInputComponents,
            limits.maxFragmentOutputAttachments,
            limits.maxFragmentDualSrcAttachments,
            limits.maxFragmentCombinedOutputResources,
            limits.maxComputeSharedMemorySize,
            mpl::to_array(limits.maxComputeWorkGroupCount),
            limits.maxComputeWorkGroupInvocations,
            mpl::to_array(limits.maxComputeWorkGroupSize),
            limits.subPixelPrecisionBits,
            limits.subTexelPrecisionBits,
            limits.mipmapPrecisionBits,
            limits.maxDrawIndexedIndexValue,
            limits.maxDrawIndirectCount,
            limits.maxSamplerLodBias,
            limits.maxSamplerAnisotropy,
            limits.maxViewports,
            mpl::to_array(limits.maxViewportDimensions),
            mpl::to_array(limits.viewportBoundsRange),
            limits.viewportSubPixelBits,
            limits.minMemoryMapAlignment,
            limits.minTexelBufferOffsetAlignment,
            limits.minUniformBufferOffsetAlignment,
            limits.minStorageBufferOffsetAlignment,
            limits.minTexelOffset,
            limits.maxTexelOffset,
            limits.minTexelGatherOffset,
            limits.maxTexelGatherOffset,
            limits.minInterpolationOffset,
            limits.maxInterpolationOffset,
            limits.subPixelInterpolationOffsetBits,
            limits.maxFramebufferWidth,
            limits.maxFramebufferHeight,
            limits.maxFramebufferLayers,
            limits.framebufferColorSampleCounts,
            limits.framebufferDepthSampleCounts,
            limits.framebufferStencilSampleCounts,
            limits.framebufferNoAttachmentsSampleCounts,
            limits.maxColorAttachments,
            limits.sampledImageColorSampleCounts,
            limits.sampledImageIntegerSampleCounts,
            limits.sampledImageDepthSampleCounts,
            limits.sampledImageStencilSampleCounts,
            limits.storageImageSampleCounts,
            limits.maxSampleMaskWords,
            limits.timestampComputeAndGraphics == VK_TRUE,
            limits.timestampPeriod,
            limits.maxClipDistances,
            limits.maxCullDistances,
            limits.maxCombinedClipAndCullDistances,
            limits.discreteQueuePriorities,
            mpl::to_array(limits.pointSizeRange),
            mpl::to_array(limits.lineWidthRange),
            limits.pointSizeGranularity,
            limits.lineWidthGranularity,
            limits.strictLines == VK_TRUE,
            limits.standardSampleLocations == VK_TRUE,
            limits.optimalBufferCopyOffsetAlignment,
            limits.optimalBufferCopyRowPitchAlignment,
            limits.nonCoherentAtomSize
        };
    }
}

namespace vulkan
{
    device::device(vulkan::instance &instance, VkSurfaceKHR surface)
    {
        auto constexpr use_extensions = !vulkan::device_extensions.empty();

        std::vector<std::string_view> extensions_view;
        std::vector<char const *> extensions;

        if constexpr (use_extensions) {
            auto const _extensions = vulkan::device_extensions;

            std::copy(_extensions.begin(), _extensions.end(), std::back_inserter(extensions));

        #if USE_DEBUG_MARKERS
            extensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        #endif

            std::copy(std::begin(extensions), std::end(extensions), std::back_inserter(extensions_view));
        }

        graphics_queues_.resize(1);
        compute_queues_.resize(1);
        transfer_queues_.resize(1);
        presentation_queues_.resize(1);

        required_device_queues required_queues{
            graphics_queues_, compute_queues_, transfer_queues_, presentation_queues_
        };

        physical_handle_ = pick_physical_device(instance.handle(), surface, std::move(extensions_view), required_queues);

        std::vector<std::vector<float>> priorities;

        auto queue_infos = get_queue_infos(physical_handle_, surface, std::move(extensions), required_queues, priorities);

        auto const device_features = vulkan::device_features;

        VkDeviceCreateInfo const device_info{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            nullptr, 0,
            static_cast<std::uint32_t>(std::size(queue_infos)), std::data(queue_infos),
            0, nullptr,
            static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions),
            &device_features
        };

        if (auto result = vkCreateDevice(physical_handle_, &device_info, nullptr, &handle_); result != VK_SUCCESS)
            throw std::runtime_error("failed to create logical device: "s + std::to_string(result));

        get_device_queues(handle_, required_queues);

        device_limits_ = get_device_limits(physical_handle_);
    }

    device::~device()
    {
        if (handle_) {
            vkDeviceWaitIdle(handle_);
            vkDestroyDevice(handle_, nullptr);
        }

        handle_ = nullptr;
        physical_handle_ = nullptr;
    }
}