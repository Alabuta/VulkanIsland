#include <algorithm>
#include <iostream>
#include <variant>
#include <vector>
#include <array>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "utility/helpers.hxx"
#include "utility/exceptions.hxx"
#include "utility/mpl.hxx"

#include "vulkan_config.hxx"
#include "device_config.hxx"
#include "device.hxx"
#include "graphics/graphics_api.hxx"
#include "renderer/swapchain.hxx"


namespace
{
    auto constexpr queue_strict_matching = false;

    using queue_t = std::variant<graphics::graphics_queue, graphics::compute_queue, graphics::transfer_queue>;

    using device_extended_feature_t = mpl::variant_from_tuple<std::remove_cvref_t<decltype(vulkan::device_extended_features)>>::type;

    auto constexpr possible_surface_formats = std::array{
        graphics::FORMAT::BGRA8_SRGB,
        graphics::FORMAT::BGRA8_UNORM,
        graphics::FORMAT::RGBA8_SRGB,
        graphics::FORMAT::RGBA8_UNORM,
        graphics::FORMAT::R5G6B5_UNORM_PACK16,
        graphics::FORMAT::RGBA8_SNORM,
        graphics::FORMAT::ABGR8_UNORM_PACK32,
        graphics::FORMAT::ABGR8_SNORM_PACK32,
        graphics::FORMAT::ABGR8_SRGB_PACK32,
        graphics::FORMAT::A2RGB10_UNORM_PACK32,
        graphics::FORMAT::A2BGR10_UNORM_PACK32,
        graphics::FORMAT::RGBA16_UNORM,
        graphics::FORMAT::RGBA16_SNORM,
        graphics::FORMAT::B10GR11_UFLOAT_PACK32,
        graphics::FORMAT::B5G6R5_UNORM_PACK16,
        graphics::FORMAT::BGRA8_SNORM,
        graphics::FORMAT::RGBA16_SFLOAT,
        graphics::FORMAT::A1RGB5_UNORM_PACK16,
        graphics::FORMAT::RGBA4_UNORM_PACK16,
        graphics::FORMAT::BGRA4_UNORM_PACK16,
        graphics::FORMAT::RGB5A1_UNORM_PACK16,
        graphics::FORMAT::BGR5A1_UNORM_PACK16
    };

    auto constexpr possible_surface_color_spaces = std::array{
        graphics::COLOR_SPACE::SRGB_NONLINEAR,
        graphics::COLOR_SPACE::EXTENDED_SRGB_LINEAR,
        graphics::COLOR_SPACE::EXTENDED_SRGB_NONLINEAR,
        graphics::COLOR_SPACE::BT709_LINEAR,
        graphics::COLOR_SPACE::BT709_NONLINEAR,
        graphics::COLOR_SPACE::ADOBE_RGB_LINEAR,
        graphics::COLOR_SPACE::ADOBE_RGB_NONLINEAR,
        graphics::COLOR_SPACE::HDR10_ST2084,
        graphics::COLOR_SPACE::HDR10_HLG,
        graphics::COLOR_SPACE::PASS_THROUGH
    };

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
            std::copy_n(std::begin(name), std::size(name), prop.extensionName);

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
            throw vulkan::device_exception(fmt::format("failed to retrieve device extensions count: {0:#x}"s, result));

        std::vector<VkExtensionProperties> supported_extensions(extensions_count);

        if (auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, std::data(supported_extensions)); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve device extensions: {0:#x}"s, result));

        std::sort(std::begin(supported_extensions), std::end(supported_extensions), extensions_compare);

        std::vector<VkExtensionProperties> unsupported_extensions;

        std::set_difference(std::begin(required_extensions), std::end(required_extensions), std::begin(supported_extensions),
                            std::end(supported_extensions), std::back_inserter(unsupported_extensions), extensions_compare);

        if (unsupported_extensions.empty())
            return true;

        std::cerr << "unsupported device extensions: "s << std::endl;

        for (auto &&extension : unsupported_extensions)
            std::cerr << fmt::format("{}\n"s, extension.extensionName);

        return false;
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
            return std::visit([&] <class T> (T const &lhs)
            {
                return overloaded_compare(lhs, std::get<T>(rhs));
            }, lhs);
        };

        return std::equal(std::cbegin(required_extended_features), std::cend(required_extended_features),
                          std::cbegin(supported_extended_features), compare);
    }

    renderer::swapchain_support_details query_swapchain_support_details(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        VkSurfaceCapabilitiesKHR surface_capabilities;

        if (auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve device surface capabilities: {0:#x}"s, result));

        std::uint32_t surface_formats_count = 0;

        if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_formats_count, nullptr); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve device surface formats count: {0:#x}"s, result));

        if (surface_formats_count == 0)
            throw vulkan::device_exception("zero number of presentation format pairs"s);

        std::vector<VkSurfaceFormatKHR> supported_formats(surface_formats_count);

        if (auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_formats_count, std::data(supported_formats)); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve device surface formats: {0:#x}"s, result));

        std::vector<renderer::surface_format> surface_formats;

        std::transform(std::cbegin(supported_formats), std::cend(supported_formats), std::back_inserter(surface_formats), [] (auto supported)
        {
            auto it_color_space = std::find_if(std::cbegin(possible_surface_color_spaces), std::cend(possible_surface_color_spaces), [supported] (auto color_space)
            {
                return convert_to::vulkan(color_space) == supported.colorSpace;
            });

            if (it_color_space == std::cend(possible_surface_color_spaces))
                throw vulkan::device_exception("failed to find matching device surface color space"s);

            auto it_format = std::find_if(std::cbegin(possible_surface_formats), std::cend(possible_surface_formats), [supported] (auto format)
            {
                return convert_to::vulkan(format) == supported.format;
            });

            if (it_format == std::cend(possible_surface_formats))
                throw vulkan::device_exception("failed to find matching device surface format"s);

            return renderer::surface_format{ *it_format, *it_color_space };
        });

        std::uint32_t present_modes_count = 0;

        if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, nullptr); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve device surface presentation modes count: {0:#x}"s, result));

        if (present_modes_count == 0)
            throw vulkan::device_exception("zero number of presentation modes"s);

        std::vector<VkPresentModeKHR> supported_modes(present_modes_count);

        if (auto result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, std::data(supported_modes)); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve device surface presentation modes: {0:#x}"s, result));

        std::vector<graphics::PRESENTATION_MODE> presentation_modes{
            graphics::PRESENTATION_MODE::IMMEDIATE,
            graphics::PRESENTATION_MODE::MAILBOX,
            graphics::PRESENTATION_MODE::FIFO,
            graphics::PRESENTATION_MODE::FIFO_RELAXED
        };

        auto it_end = std::remove_if(std::begin(presentation_modes), std::end(presentation_modes), [&supported_modes] (auto mode)
        {
            return std::none_of(std::begin(supported_modes), std::end(supported_modes), [mode] (auto supported_mode)
            {
                return supported_mode == convert_to::vulkan(mode);
            });
        });

        presentation_modes.erase(it_end, std::end(presentation_modes));
        presentation_modes.shrink_to_fit();

        return { surface_capabilities, surface_formats, presentation_modes };
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
                    throw vulkan::device_exception(fmt::format("failed to retrieve surface support: {0:#x}"s, result));

                if (surface_supported != VK_TRUE)
                    return false;
            }

            auto const capability = ([]
            {
                if constexpr (std::is_same_v<T, graphics::graphics_queue>)
                    return convert_to::vulkan(graphics::QUEUE_CAPABILITY::GRAPHICS);

                else if constexpr (std::is_same_v<T, graphics::compute_queue>)
                    return convert_to::vulkan(graphics::QUEUE_CAPABILITY::COMPUTE);

                else if constexpr (std::is_same_v<T, graphics::transfer_queue>)
                    return convert_to::vulkan(graphics::QUEUE_CAPABILITY::TRANSFER);

                static_assert(mpl::is_one_of_v<T, graphics::graphics_queue, graphics::compute_queue, graphics::transfer_queue>);
            })();

            using C = std::remove_cvref_t<decltype(queue_family.queueFlags)>;

            if constexpr (strict_matching)
                return queue_family.queueCount > 0 && queue_family.queueFlags == capability;

            return queue_family.queueCount > 0 && (queue_family.queueFlags & static_cast<C>(capability)) == static_cast<C>(capability);
        });

        if (it_family != std::cend(queue_families))
            return queue_family{ static_cast<std::uint32_t>(std::distance(std::cbegin(queue_families), it_family)), *it_family };

        return { };
    }

    VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface, std::vector<std::string_view> &&extensions)
    {
        std::uint32_t devices_count = 0;

        if (auto result = vkEnumeratePhysicalDevices(instance, &devices_count, nullptr); result != VK_SUCCESS || devices_count == 0)
            throw vulkan::device_exception(fmt::format("failed to find physical device with Vulkan API support: {0:#x}"s, result));

        std::vector<VkPhysicalDevice> devices(devices_count);

        if (auto result = vkEnumeratePhysicalDevices(instance, &devices_count, std::data(devices)); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve physical devices: {0:#x}"s, result));

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
                &supported_extended_features.back(),
                { }
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
            auto details = query_swapchain_support_details(device, surface);

            return details.surface_formats.empty() || details.presentation_modes.empty();
        });

        devices.erase(it_end, std::end(devices));

        if (devices.empty())
            throw vulkan::device_exception("failed to pick physical device"s);

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
            limits.bufferImageGranularity,
            limits.sparseAddressSpaceSize,
            limits.minTexelBufferOffsetAlignment,
            limits.minUniformBufferOffsetAlignment,
            limits.minStorageBufferOffsetAlignment,
            limits.optimalBufferCopyOffsetAlignment,
            limits.optimalBufferCopyRowPitchAlignment,
            limits.nonCoherentAtomSize,

            limits.minMemoryMapAlignment,

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
            limits.timestampPeriod,
            limits.maxClipDistances,
            limits.maxCullDistances,
            limits.maxCombinedClipAndCullDistances,
            limits.discreteQueuePriorities,
            mpl::to_array(limits.pointSizeRange),
            mpl::to_array(limits.lineWidthRange),
            limits.pointSizeGranularity,
            limits.lineWidthGranularity,

            limits.timestampComputeAndGraphics == VK_TRUE,
            limits.strictLines == VK_TRUE,
            limits.standardSampleLocations == VK_TRUE
        };
    }
}

namespace vulkan
{
    struct device::queue_helper final {
        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        std::vector<std::vector<float>> priorities;

        queue_helper(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<queue_t> &requested_queues)
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

                    else throw vulkan::device_exception("failed to get the queue family"s);

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
    device::device(vulkan::instance &instance, renderer::platform_surface platform_surface) : instance_{instance}
    {
        auto constexpr use_extensions = !vulkan::device_extensions.empty();

        std::vector<char const *> extensions;

        if constexpr (use_extensions) {
            auto constexpr extensions_ = vulkan::device_extensions;

            std::copy(std::begin(extensions_), std::end(extensions_), std::back_inserter(extensions));

            std::vector<std::string_view> extensions_view{std::begin(extensions), std::end(extensions)};

            physical_handle_ = pick_physical_device(instance.handle(), platform_surface.handle(), std::move(extensions_view));
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

        device::queue_helper helper(physical_handle_, platform_surface.handle(), requested_queues);

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
            throw vulkan::device_exception(fmt::format("failed to create logical device: {0:#x}"s, result));

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
        if (handle_ == VK_NULL_HANDLE)
            return;

        vkDeviceWaitIdle(handle_);

        vkDestroyDevice(handle_, nullptr);

        handle_ = nullptr;
        physical_handle_ = nullptr;
    }

    renderer::swapchain_support_details device::query_swapchain_support_details(renderer::platform_surface platform_surface) const
    {
        return ::query_swapchain_support_details(physical_handle_, platform_surface.handle());
    }

    bool device::is_format_supported_as_buffer_feature(graphics::FORMAT format, graphics::FORMAT_FEATURE features) const noexcept
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical_handle_, convert_to::vulkan(format), &properties);

        return (properties.bufferFeatures & convert_to::vulkan(features)) == convert_to::vulkan(features);
    }
}