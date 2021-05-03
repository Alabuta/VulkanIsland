#include <algorithm>
#include <iostream>
#include <variant>
#include <ranges>
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

    template<bool CheckOnDuplicates = false>
    bool check_required_device_extensions(VkPhysicalDevice physical_device, std::vector<std::string_view> &&extensions)
    {
        std::vector<VkExtensionProperties> required_extensions;

        auto extensions_compare = [] (auto &&lhs, auto &&rhs)
        {
            return std::ranges::lexicographical_compare(lhs.extensionName, rhs.extensionName);
        };

        std::ranges::transform(extensions, std::back_inserter(required_extensions), [] (auto &&name)
        {
            VkExtensionProperties prop{};
            std::copy_n(std::begin(name), std::size(name), prop.extensionName);

            return prop;
        });

        std::ranges::sort(required_extensions, extensions_compare);

        if constexpr (CheckOnDuplicates) {
            auto it = std::ranges::unique(required_extensions, [] (auto &&lhs, auto &&rhs)
            {
                return std::ranges::equal(lhs.extensionName, rhs.extensionName);
            });

            required_extensions.erase(it, std::end(required_extensions));
        }

        std::uint32_t extensions_count = 0;

        if (auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, nullptr); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve device extensions count: {0:#x}"s, result));

        std::vector<VkExtensionProperties> supported_extensions(extensions_count);

        if (auto result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, std::data(supported_extensions)); result != VK_SUCCESS)
            throw vulkan::device_exception(fmt::format("failed to retrieve device extensions: {0:#x}"s, result));

        std::ranges::sort(supported_extensions, extensions_compare);

        std::vector<VkExtensionProperties> unsupported_extensions;

        std::ranges::set_difference(required_extensions, supported_extensions, std::back_inserter(unsupported_extensions), extensions_compare);

        if (unsupported_extensions.empty())
            return true;

        std::cerr << "unsupported device extensions: "s << std::endl;

        for (auto &&extension : unsupported_extensions)
            std::cerr << fmt::format("{}\n"s, extension.extensionName);

        return false;
    }

    bool compare_device_features(VkPhysicalDeviceFeatures &lhs, VkPhysicalDeviceFeatures &rhs)
    {
        auto total = static_cast<VkBool32>(VK_TRUE);

        auto compare_fields_by_lhs = [&lhs, &rhs] (auto getter) { return getter(lhs) == (getter(lhs) * getter(rhs)); };

        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.robustBufferAccess; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.fullDrawIndexUint32; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.imageCubeArray; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.independentBlend; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.geometryShader; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.tessellationShader; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sampleRateShading; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.dualSrcBlend; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.logicOp; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.multiDrawIndirect; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.drawIndirectFirstInstance; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.depthClamp; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.depthBiasClamp; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.fillModeNonSolid; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.depthBounds; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.wideLines; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.largePoints; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.alphaToOne; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.multiViewport; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.samplerAnisotropy; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.textureCompressionETC2; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.textureCompressionASTC_LDR; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.textureCompressionBC; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.occlusionQueryPrecise; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.pipelineStatisticsQuery; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.vertexPipelineStoresAndAtomics; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.fragmentStoresAndAtomics; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderTessellationAndGeometryPointSize; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderImageGatherExtended; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderStorageImageExtendedFormats; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderStorageImageMultisample; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderStorageImageReadWithoutFormat; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderStorageImageWriteWithoutFormat; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderUniformBufferArrayDynamicIndexing; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderSampledImageArrayDynamicIndexing; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderStorageBufferArrayDynamicIndexing; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderStorageImageArrayDynamicIndexing; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderClipDistance; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderCullDistance; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderFloat64; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderInt64; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderInt16; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderResourceResidency; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.shaderResourceMinLod; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseBinding; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseResidencyBuffer; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseResidencyImage2D; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseResidencyImage3D; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseResidency2Samples; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseResidency4Samples; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseResidency8Samples; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseResidency16Samples; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.sparseResidencyAliased; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.variableMultisampleRate; });
        total *= compare_fields_by_lhs([] (auto &&feature) { return feature.inheritedQueries; });

        return total != static_cast<VkBool32>(VK_FALSE);
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

        return std::ranges::equal(required_extended_features, supported_extended_features, compare);
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

        std::ranges::transform(supported_formats, std::back_inserter(surface_formats), [] (auto supported)
        {
            auto it_color_space = std::ranges::find_if(possible_surface_color_spaces, [supported] (auto color_space)
            {
                return convert_to::vulkan(color_space) == supported.colorSpace;
            });

            if (it_color_space == std::cend(possible_surface_color_spaces))
                throw vulkan::device_exception("failed to find matching device surface color space"s);

            auto it_format = std::ranges::find_if(possible_surface_formats, [supported] (auto format)
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

        auto subrange = std::ranges::remove_if(presentation_modes, [&supported_modes] (auto mode)
        {
            return std::ranges::none_of(supported_modes, [mode] (auto supported_mode)
            {
                return supported_mode == convert_to::vulkan(mode);
            });
        });
        
        presentation_modes.erase(std::begin(subrange), std::end(subrange));
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

        auto it_family = std::ranges::find_if(queue_families, [device, surface, family_index = 0u] (auto &&queue_family) mutable
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
            return queue_family{ static_cast<std::uint32_t>(std::ranges::distance(std::cbegin(queue_families), it_family)), *it_family };

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
        auto subrange = std::ranges::remove_if(devices, [&extensions, &required_extended_features] (auto &&device)
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

            for (void *ptr_next = nullptr; auto && supported_feature : supported_extended_features) {
                std::visit([&ptr_next] (auto &&feature)
                {
                    feature.pNext = ptr_next;

                    ptr_next = &feature;
                }, supported_feature);
            }

            VkPhysicalDeviceFeatures2 supported_features{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                &supported_extended_features.back(),
                { }
            };

            vkGetPhysicalDeviceFeatures2(device, &supported_features);
            vkGetPhysicalDeviceFeatures(device, &supported_features.features); // TODO:: maybe it's a bug

            auto required_features = vulkan::device_features;

            if (!compare_device_features(required_features, supported_features.features))
                return true;

            if (!compare_device_extended_features(required_extended_features, std::move(supported_extended_features)))
                return true;

            return false;
        });

        devices.erase(std::begin(subrange), std::end(subrange));

        // Removing unsuitable devices. Matching by required compute, graphics, transfer and presentation queues.
        subrange = std::ranges::remove_if(std::begin(devices), std::end(devices), [&] (auto &&device)
        {
            if (!get_queue_family<queue_strict_matching, graphics::graphics_queue>(device, surface))
                return true;

            if (!get_queue_family<queue_strict_matching, graphics::compute_queue>(device))
                return true;

            if (!get_queue_family<queue_strict_matching, graphics::transfer_queue>(device))
                return true;

            return false;
        });

        devices.erase(std::begin(subrange), std::end(subrange));

        // Matching by the swap chain properties support.
        subrange = std::ranges::remove_if(std::begin(devices), std::end(devices), [surface] (auto &&device)
        {
            auto details = query_swapchain_support_details(device, surface);

            return details.surface_formats.empty() || details.presentation_modes.empty();
        });

        devices.erase(std::begin(subrange), std::end(subrange));

        if (devices.empty())
            throw vulkan::device_exception("failed to pick physical device"s);

        auto constexpr device_types_priority = std::array{
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
            VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_CPU
        };

        // Sorting by device type.
        for (auto device_type : device_types_priority) {
            auto next_type_group = std::ranges::stable_partition(devices, [device_type] (auto &&device)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);

                return properties.deviceType == device_type;
            });

            if (std::begin(next_type_group) != std::end(next_type_group))
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
    device::device(vulkan::instance &instance, renderer::platform_surface platform_surface)
    {
        auto constexpr use_extensions = !vulkan::device_extensions.empty();

        std::vector<char const *> extensions;

        if constexpr (use_extensions) {
            auto constexpr extensions_ = vulkan::device_extensions;

            std::ranges::copy(extensions_, std::back_inserter(extensions));

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