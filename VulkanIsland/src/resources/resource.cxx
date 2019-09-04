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
                  VkSampleCountFlagBits samplesCount, graphics::FORMAT format, graphics::IMAGE_TILING tiling, VkBufferUsageFlags usage) noexcept
{
    VkImageCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr, 0,
        VK_IMAGE_TYPE_2D,
        convert_to::vulkan(format),
        { width, height, 1 },
        mipLevels,
        1,
        samplesCount,
        convert_to::vulkan(tiling),
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
ResourceManager::CreateImage(graphics::FORMAT format, std::uint16_t width, std::uint16_t height, std::uint32_t mipLevels,
                             VkSampleCountFlagBits samplesCount, graphics::IMAGE_TILING tiling, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    std::shared_ptr<VulkanImage> image;

    auto handle = CreateImageHandle(device_, width, height, mipLevels, samplesCount, format, tiling, usageFlags);

    if (handle) {
        auto const linearMemory = tiling == graphics::IMAGE_TILING::LINEAR;

        auto memory = device_.memoryManager().AllocateMemory(*handle, propertyFlags, linearMemory);

        if (memory) {
            if (auto result = vkBindImageMemory(device_.handle(), *handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
                std::cerr << "failed to bind image buffer memory: "s << result << '\n';

            else image.reset(
                new VulkanImage{memory, *handle, format, tiling, mipLevels, width, height},
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
        convert_to::vulkan(image.format()),
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
        auto const linearMemory = true;

        auto memory = device_.memoryManager().AllocateMemory(*handle, properties, linearMemory);

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

template<class T, std::enable_if_t<is_one_of_v<std::decay_t<T>, VulkanImage, VulkanSampler, VulkanImageView, VulkanBuffer, VulkanShaderModule>>...>
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

std::shared_ptr<VertexBuffer> ResourceManager::CreateVertexBuffer(xformat::vertex_layout const &layout, std::size_t sizeInBytes) noexcept
{
    if (vertexBuffers_.count(layout) == 0) {
        std::shared_ptr<VulkanBuffer> stagingBuffer;
        std::shared_ptr<VulkanBuffer> deviceBuffer;
        
        auto const capacityInBytes = sizeInBytes * kVertexBufferIncreaseValue;

        {
            auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            stagingBuffer = device_.resourceManager().CreateBuffer(sizeInBytes, usageFlags, propertyFlags);

            if (!stagingBuffer) {
                std::cerr << "failed to create staging vertex buffer\n"s;
                return { };
            }
        }

        {
            auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            deviceBuffer = device_.resourceManager().CreateBuffer(capacityInBytes, usageFlags, propertyFlags);

            if (!deviceBuffer) {
                std::cerr << "failed to create device vertex buffer\n"s;
                return { };
            }
        }

        vertexBuffers_.emplace(layout, std::make_shared<VertexBuffer>(deviceBuffer, stagingBuffer, capacityInBytes, layout));
    }

    auto &vertexBuffer = vertexBuffers_[layout];

    if (vertexBuffer->stagingBufferSizeInBytes_ < sizeInBytes) {
        auto constexpr usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        vertexBuffer->stagingBuffer_ = device_.resourceManager().CreateBuffer(sizeInBytes, usageFlags, propertyFlags);

        if (!vertexBuffer->stagingBuffer_) {
            std::cerr << "failed to extend staging vertex buffer\n"s;
            return { };
        }

        vertexBuffer->stagingBufferSizeInBytes_ = sizeInBytes;
    }

    if (vertexBuffer->availableMemorySize() < sizeInBytes) {
        // TODO: sparse memory binding
#if NOT_YET_IMPLEMENTED
        auto bufferHandle = vertexBuffer->deviceBuffer_->handle();
        auto memory = device_.memoryManager().AllocateMemory(bufferHandle, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#endif
        std::cerr << "not enough device memory for vertex buffer\n"s;
        return { };
    }

    return vertexBuffer;
}

void ResourceManager::StageVertexData(std::shared_ptr<VertexBuffer> vertexBuffer, std::vector<std::byte> const &container) const
{
    if (vertexBuffer) {
        auto const lengthInBytes = std::size(container);

        // TODO: sparse memory binding
        if (vertexBuffer->availableMemorySize() < lengthInBytes)
            throw std::runtime_error("not enough device memory for vertex buffer"s);

        auto &&memory = vertexBuffer->stagingBuffer().memory();

        void *ptr;

        if (auto result = vkMapMemory(device_.handle(), memory->handle(), vertexBuffer->stagingBufferOffset(), lengthInBytes, 0, &ptr); result != VK_SUCCESS)
            throw std::runtime_error("failed to map staging vertex buffer memory: "s + std::to_string(result));

        else {
            std::uninitialized_copy_n(std::begin(container), lengthInBytes, reinterpret_cast<std::byte *>(ptr));

            vkUnmapMemory(device_.handle(), memory->handle());

            vertexBuffer->offset_ += lengthInBytes;
        }
    }
}

void ResourceManager::TransferStagedVertexData(VkCommandPool transferCommandPool, TransferQueue &transferQueue) const
{
    for (auto &&[layout, vertexBuffer] : vertexBuffers_) {
        auto &&stagingBuffer = vertexBuffer->stagingBuffer();
        auto &&deviceBuffer = vertexBuffer->deviceBuffer();

        auto copyRegions = std::array{VkBufferCopy{ 0, 0, stagingBuffer.memory()->size() }};

        CopyBufferToBuffer(device_, transferQueue, stagingBuffer.handle(), deviceBuffer.handle(), std::move(copyRegions), transferCommandPool);
    }
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
