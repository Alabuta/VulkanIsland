#include <algorithm>
#include <variant>
#include <vector>
#include <array>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "utility/helpers.hxx"
#include "utility/mpl.hxx"

#define USE_DEBUG_MARKERS 0

#include "vulkan_config.hxx"
#include "device_config.hxx"
#include "device.hxx"
#include "renderer/swapchain.hxx"

// TODO:: remove
#include "swapchain_old.hxx"


namespace
{
    auto constexpr queue_strict_matching = false;

    using queue_t = std::variant<graphics::graphics_queue, graphics::compute_queue, graphics::transfer_queue>;

    using device_extended_feature_t = mpl::variant_from_tuple<std::remove_cvref_t<decltype(vulkan::device_extended_features)>>::type;

    template<bool check_on_duplicates = false>
    bool check_required_device_extensions(VkPhysicalDevice physical_device, std::vector<std::string_view> &&extensions)
    {
        std::vector<VkExtensionProperties> required_extensions;

        auto extensions_compare = [] (auto &&lhs, auto &&rhs)
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

    bool compare_device_features(VkPhysicalDeviceFeatures &&lhs, VkPhysicalDeviceFeatures &&rhs)
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

    bool compare_device_extended_features(std::vector<device_extended_feature_t> const &required_extended_features,
                                          std::vector<device_extended_feature_t> &&supported_extended_features)
    {
        auto overloaded_compare = overloaded{
            [] (VkPhysicalDevice8BitStorageFeaturesKHR const &lhs, VkPhysicalDevice8BitStorageFeaturesKHR const &rhs)
            {
                if (lhs.storageBuffer8BitAccess != (lhs.storageBuffer8BitAccess * rhs.storageBuffer8BitAccess))
                    return false;

                if (lhs.uniformAndStorageBuffer8BitAccess != (lhs.uniformAndStorageBuffer8BitAccess * rhs.uniformAndStorageBuffer8BitAccess))
                    return false;

                if (lhs.storagePushConstant8 != (lhs.storagePushConstant8 * rhs.storagePushConstant8))
                    return false;

                return true;
            },
            [] (VkPhysicalDevice16BitStorageFeatures const &lhs, VkPhysicalDevice16BitStorageFeatures const &rhs)
            {
                if (lhs.storageBuffer16BitAccess != (lhs.storageBuffer16BitAccess * rhs.storageBuffer16BitAccess))
                    return false;

                if (lhs.uniformAndStorageBuffer16BitAccess != (lhs.uniformAndStorageBuffer16BitAccess * rhs.uniformAndStorageBuffer16BitAccess))
                    return false;

                if (lhs.storagePushConstant16 != (lhs.storagePushConstant16 * rhs.storagePushConstant16))
                    return false;

                if (lhs.storageInputOutput16 != (lhs.storageInputOutput16 * rhs.storageInputOutput16))
                    return false;

                return true;
            },
            [] (VkPhysicalDeviceFloat16Int8FeaturesKHR const &lhs, VkPhysicalDeviceFloat16Int8FeaturesKHR const &rhs)
            {
                if (lhs.shaderFloat16 != (lhs.shaderFloat16 * rhs.shaderFloat16))
                    return false;

                if (lhs.shaderInt8 != (lhs.shaderInt8 * rhs.shaderInt8))
                    return false;

                return true;
            },
        };

        auto compare = [&overloaded_compare] (auto &&lhs, auto &&rhs)
        {
            return std::visit([&] (auto &&lhs)
            {
                using T = std::remove_cvref_t<decltype(lhs)>;

                return overloaded_compare(lhs, std::get<T>(rhs));
            }, lhs);
        };

        return std::equal(std::cbegin(required_extended_features), std::cend(required_extended_features),
                          std::cbegin(supported_extended_features), compare);
    }

    struct queue_family final {
        std::uint32_t index;
        VkQueueFamilyProperties property;
    };

    template<bool strict_matching, class T>
    std::optional<queue_family> get_queue_family(VkPhysicalDevice device, VkSurfaceKHR surface = VK_NULL_HANDLE)
    {
        std::uint32_t queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, nullptr);

        if (queue_families_count == 0)
            return { };

        std::vector<VkQueueFamilyProperties> queue_families(queue_families_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, std::data(queue_families));

        auto it_family = std::find_if(std::cbegin(queue_families), std::cend(queue_families),
                                      [device, surface, family_index = 0u] (auto &&queue_family) mutable
        {
            if constexpr (std::is_same_v<T, graphics::graphics_queue>) {
                VkBool32 surface_supported = VK_FALSE;

                if (auto result = vkGetPhysicalDeviceSurfaceSupportKHR(device, family_index++, surface, &surface_supported); result != VK_SUCCESS)
                    throw std::runtime_error(fmt::format("failed to retrieve surface support: {0:#x}\n"s, result));

                if (surface_supported != VK_TRUE)
                    return false;
            }

            auto const capability = convert_to::vulkan(T::capability);

            if constexpr (strict_matching)
                return queue_family.queueCount > 0 && queue_family.queueFlags == capability;

            return queue_family.queueCount > 0 && (queue_family.queueFlags & capability) == capability;
        });

        if (it_family != std::cend(queue_families))
            return queue_family{ static_cast<std::uint32_t>(std::distance(std::cbegin(queue_families), it_family)), *it_family };

        return { };
    }

    VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extensions)
    {
        std::uint32_t devices_count = 0;

        if (auto result = vkEnumeratePhysicalDevices(instance, &devices_count, nullptr); result != VK_SUCCESS || devices_count == 0)
            throw std::runtime_error(fmt::format("failed to find physical device with Vulkan API support: {0:#x}\n"s, result));

        std::vector<VkPhysicalDevice> devices(devices_count);

        if (auto result = vkEnumeratePhysicalDevices(instance, &devices_count, std::data(devices)); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to retrieve physical devices: {0:#x}\n"s, result));

        auto const required_extended_features = std::apply([] (auto ...args)
        {
            return std::vector<device_extended_feature_t>{std::move(args)...};
        }, vulkan::device_extended_features);

        // Matching by supported properties, features and extensions.
        auto it_end = std::remove_if(std::begin(devices), std::end(devices),
                                     [&extensions, &required_extended_features] (auto &&device)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            if (properties.apiVersion < vulkan_config::application_info.apiVersion)
                return true;

            if (!check_required_device_extensions(device, std::move(extensions)))
                return true;

            auto supported_extended_features = std::apply([] (auto ...args)
            {
                return std::vector<device_extended_feature_t>{args...};
            }, vulkan::device_extended_features);

            void *ptr_next = nullptr;

            for (auto &&feature : supported_extended_features) {
                std::visit([&ptr_next] (auto &&feature)
                {
                    feature.pNext = ptr_next;

                    ptr_next = &feature;
                }, feature);
            }

            VkPhysicalDeviceFeatures2 supported_features{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                &supported_extended_features.back()
            };

            vkGetPhysicalDeviceFeatures2(device, &supported_features);
            vkGetPhysicalDeviceFeatures(device, &supported_features.features); // TODO:: maybe it's a bug

            auto required_features = vulkan::device_features;

            if (!compare_device_features(std::move(required_features), std::move(supported_features.features)))
                return true;

            if (!compare_device_extended_features(required_extended_features, std::move(supported_extended_features)))
                return true;

            return false;
        });

        devices.erase(it_end, std::end(devices));

        // Removing unsuitable devices. Matching by required compute, graphics, transfer and presentation queues.
        it_end = std::remove_if(std::begin(devices), std::end(devices), [&] (auto &&device)
        {
            if (!get_queue_family<queue_strict_matching, graphics::graphics_queue>(device, surface))
                return true;

            if (!get_queue_family<queue_strict_matching, graphics::compute_queue>(device))
                return true;

            if (!get_queue_family<queue_strict_matching, graphics::transfer_queue>(device))
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

    vulkan::device_limits get_device_limits(VkPhysicalDevice physical_handle)
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
    struct device::helper final {
        device::helper(std::vector<queue_t> &requested_queues) noexcept : requested_queues{requested_queues} { }

        std::vector<queue_t> &requested_queues;

        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        std::vector<std::vector<float>> priorities;

        void init_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            std::map<std::uint32_t, std::uint32_t> requested_families;

            for (auto &&queue : requested_queues) {
                std::visit([device, surface, &requested_families] (auto &&queue)
                {
                    using T = std::remove_cvref_t<decltype(queue)>;

                    if (auto queue_family = get_queue_family<queue_strict_matching, T>(device, surface); queue_family) {
                        auto &&[family_index, family_property] = *queue_family;

                        queue.family_ = family_index;
                        queue.index_ = requested_families[family_index];

                        ++requested_families[family_index];
                    }

                    else throw std::runtime_error("failed to get the queue family"s);

                }, queue);
            }

            for (auto [family, count] : requested_families) {
                priorities.emplace_back(count, 1.f);

                queue_infos.push_back(VkDeviceQueueCreateInfo{
                    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    nullptr, 0,
                    family, count,
                    std::data(priorities.back())
                });
            }
        }
    };
}

namespace vulkan
{
    device::device(vulkan::instance &instance, renderer::platform_surface const *const platform_surface)
    {
        auto constexpr use_extensions = !vulkan::device_extensions.empty();

        std::vector<char const *> extensions;

        if constexpr (use_extensions) {
            auto const _extensions = vulkan::device_extensions;

            std::copy(std::begin(_extensions), std::end(_extensions), std::back_inserter(extensions));

        #if USE_DEBUG_MARKERS
            extensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        #endif

            std::vector<std::string_view> extensions_view{std::begin(extensions), std::end(extensions)};

            physical_handle_ = pick_physical_device(instance.handle(), platform_surface->handle(), std::move(extensions_view));
        }

        auto required_extended_features = std::apply([] (auto ...args)
        {
            return std::vector<device_extended_feature_t>{std::move(args)...};
        }, vulkan::device_extended_features);

        void *ptr_next = nullptr;

        for (auto &&feature : required_extended_features) {
            std::visit([&ptr_next] (auto &&feature)
            {
                feature.pNext = ptr_next;

                ptr_next = &feature;
            }, feature);
        }

        VkPhysicalDeviceFeatures2 required_device_features{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            &required_extended_features.back(),
            vulkan::device_features
        };

        std::vector<queue_t> requested_queues{
            graphics_queue, compute_queue, transfer_queue, presentation_queue
        };

        device::helper helper(requested_queues);

        helper.init_queue_families(physical_handle_, platform_surface->handle());

        VkDeviceCreateInfo const device_info{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            &required_device_features,
            0,
            static_cast<std::uint32_t>(std::size(helper.queue_infos)), std::data(helper.queue_infos),
            0, nullptr,
            static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions),
            nullptr
        };

        if (auto result = vkCreateDevice(physical_handle_, &device_info, nullptr, &handle_); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create logical device: {0:#x}\n"s, result));

        for (auto &&queue : requested_queues) {
            std::visit([this] (auto &&queue)
            {
                using T = std::remove_cvref_t<decltype(queue)>;

                vkGetDeviceQueue(handle_, queue.family(), queue.index(), &queue.handle_);

                if constexpr (std::is_same_v<T, graphics::graphics_queue>) {
                    if (graphics_queue.handle() == VK_NULL_HANDLE)
                        graphics_queue = queue;

                    else presentation_queue = queue;
                }

                else if constexpr (std::is_same_v<T, graphics::compute_queue>)
                    compute_queue = queue;

                else if constexpr (std::is_same_v<T, graphics::transfer_queue>)
                    transfer_queue = queue;

            }, queue);
        }

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