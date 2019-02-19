#include "buffer.hxx"
#include "image.hxx"
#include "resource.hxx"
#include "commandBuffer.hxx"


namespace
{
[[nodiscard]] std::optional<VkBuffer>
CreateBufferHandle(VulkanDevice const &device, VkDeviceSize size, VkBufferUsageFlags usage) noexcept
{
    std::optional<VkBuffer> buffer;

    VkBufferCreateInfo const bufferCreateInfo{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr, 0,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr
    };

    VkBuffer handle;

    if (auto result = vkCreateBuffer(device.handle(), &bufferCreateInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create buffer: "s << result << '\n';

    else buffer.emplace(handle);

    return buffer;
}

[[nodiscard]] std::optional<VkImage>
CreateImageHandle(VulkanDevice const &vulkanDevice, std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels,
                  VkSampleCountFlagBits samplesCount, VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage) noexcept
{
    VkImageCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr, 0,
        VK_IMAGE_TYPE_2D,
        format,
        { width, height, 1 },
        mipLevels,
        1,
        samplesCount,
        tiling,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    std::optional<VkImage> image;

    VkImage handle;

    if (auto result = vkCreateImage(vulkanDevice.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create image: "s << result << '\n';

    else image.emplace(handle);

#if USE_DEBUG_MARKERS
    VkDebugMarkerObjectNameInfoEXT const info{
        VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
        (std::uint64_t)handle,//static_cast<std::uint64_t>(handle),
        "image"
    };

    auto vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(vulkanDevice.handle(), "vkDebugMarkerSetObjectNameEXT");

    if (auto result = vkDebugMarkerSetObjectNameEXT(vulkanDevice.handle(), &info); result != VK_SUCCESS)
        throw std::runtime_error("failed to set the image debug marker: "s + std::to_string(result));
#endif

    return image;
}
}

std::shared_ptr<VulkanImage>
ResourceManager::CreateImage(VkFormat format, std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels,
                             VkSampleCountFlagBits samplesCount, VkImageTiling tiling, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    std::shared_ptr<VulkanImage> image;

    auto handle = CreateImageHandle(device_, width, height, mipLevels, samplesCount, format, tiling, usageFlags);

    if (handle) {
        auto memory = device_.memoryManager().AllocateMemory(*handle, propertyFlags, tiling == VK_IMAGE_TILING_LINEAR);

        if (memory) {
            if (auto result = vkBindImageMemory(device_.handle(), *handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
                std::cerr << "failed to bind image buffer memory: "s << result << '\n';

            else image.reset(
                new VulkanImage{memory, *handle, format, mipLevels, width, height},
                [this] (VulkanImage *ptr_image)
                {
                    ReleaseResource(*ptr_image);

                    delete ptr_image;
                }
            );
        }
    }

    return image;
}

std::optional<VulkanImageView>
ResourceManager::CreateImageView(VulkanImage const &image, VkImageViewType type, VkImageAspectFlags aspectFlags) noexcept
{
    std::optional<VulkanImageView> view;

    VkImageViewCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr, 0,
        image.handle(),
        type,
        image.format(),
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        { aspectFlags, 0, image.mipLevels(), 0, 1 }
    };

    VkImageView handle;

    if (auto result = vkCreateImageView(device_.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create image view: "s << result << '\n';

    else view.emplace(handle, type);

    return view;
}

std::shared_ptr<VulkanSampler>
ResourceManager::CreateImageSampler(std::uint32_t mipLevels) noexcept
{
    std::shared_ptr<VulkanSampler> sampler;

    VkSamplerCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        nullptr, 0,
        VK_FILTER_LINEAR, VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        0.f,
        VK_TRUE, 16.f,
        VK_FALSE, VK_COMPARE_OP_ALWAYS,
        0.f, static_cast<float>(mipLevels),
        VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        VK_FALSE
    };

    VkSampler handle;

    if (auto result = vkCreateSampler(device_.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create sampler: "s << result << '\n';

    else sampler.reset(
        new VulkanSampler{handle},
        [this] (VulkanSampler *ptr_sampler)
        {
            ReleaseResource(*ptr_sampler);

            delete ptr_sampler;
        }
    );

    return sampler;
}


std::shared_ptr<VulkanBuffer>
ResourceManager::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) noexcept
{
    std::shared_ptr<VulkanBuffer> buffer;

    auto handle = CreateBufferHandle(device_, size, usage);

    if (handle) {
        auto memory = device_.memoryManager().AllocateMemory(*handle, properties, false);

        if (memory) {
            if (auto result = vkBindBufferMemory(device_.handle(), *handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
                std::cerr << "failed to bind buffer memory: "s << result << '\n';

            else buffer.reset(
                new VulkanBuffer{memory, *handle},
                [this] (VulkanBuffer *ptr_buffer)
                {
                    ReleaseResource(*ptr_buffer);

                    delete ptr_buffer;
                }
            );
        }
    }

    return buffer;
}

std::shared_ptr<VulkanShaderModule>
ResourceManager::CreateShaderModule(std::vector<std::byte> const &shaderByteCode) noexcept
{
    std::shared_ptr<VulkanShaderModule> shaderModule;

    if (shaderByteCode.size() % sizeof(std::uint32_t) != 0)
        std::cerr << "invalid byte code buffer size\n"s;

    else {
        VkShaderModuleCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            nullptr, 0,
            std::size(shaderByteCode),
            reinterpret_cast<std::uint32_t const *>(std::data(shaderByteCode))
        };

        VkShaderModule handle;

        if (auto result = vkCreateShaderModule(device_.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
            std::cerr << "failed to create shader module: "s << result << '\n';

        else shaderModule.reset(
            new VulkanShaderModule{handle},
            [this] (VulkanShaderModule *ptr_module)
            {
                ReleaseResource(*ptr_module);

                delete ptr_module;
            }
        );
    }

    return shaderModule;
}

template<class T, std::enable_if_t<is_one_of_v<std::decay_t<T>, VulkanImage, VulkanSampler, VulkanImageView, VulkanBuffer, VulkanShaderModule>> ...>
void ResourceManager::ReleaseResource(T &&resource) noexcept
{
    using R = std::decay_t<T>;

    if constexpr (std::is_same_v<R, VulkanImage>)
    {
        vkDestroyImage(device_.handle(), resource.handle(), nullptr);
        resource.memory().reset();
    }

    else if constexpr (std::is_same_v<R, VulkanSampler>)
    {
        vkDestroySampler(device_.handle(), resource.handle(), nullptr);
    }

    else if constexpr (std::is_same_v<R, VulkanImageView>)
    {
        vkDestroyImageView(device_.handle(), resource.handle(), nullptr);
    }

    else if constexpr (std::is_same_v<R, VulkanBuffer>)
    {
        vkDestroyBuffer(device_.handle(), resource.handle(), nullptr);
        resource.memory().reset();
    }

    else if constexpr (std::is_same_v<R, VulkanShaderModule>)
    {
        vkDestroyShaderModule(device_.handle(), resource.handle(), nullptr);
    }
}

std::optional<VertexBuffer> ResourceManager::CreateVertexBuffer(xformat::vertex_layout const &layout, std::size_t sizeInBytes) noexcept
{
    return std::optional<VertexBuffer>();
}

std::shared_ptr<VulkanBuffer>
CreateUniformBuffer(VulkanDevice &device, std::size_t size)
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return device.resourceManager().CreateBuffer(size, usageFlags, propertyFlags);
}

std::shared_ptr<VulkanBuffer>
CreateCoherentStorageBuffer(VulkanDevice &device, std::size_t size)
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return device.resourceManager().CreateBuffer(size, usageFlags, propertyFlags);
}

std::shared_ptr<VulkanBuffer>
CreateStorageBuffer(VulkanDevice &device, std::size_t size)
{
    auto constexpr usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    return device.resourceManager().CreateBuffer(size, usageFlags, propertyFlags);
}
