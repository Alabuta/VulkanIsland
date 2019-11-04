#include <cmath>

#include "buffer.hxx"
#include "image.hxx"
#include "resource.hxx"
#include "renderer/command_buffer.hxx"


namespace
{
[[nodiscard]] std::optional<VkBuffer>
CreateBufferHandle(vulkan::device const &device, VkDeviceSize size, graphics::BUFFER_USAGE usage) noexcept
{
    std::optional<VkBuffer> buffer;

    VkBufferCreateInfo const bufferCreateInfo{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr, 0,
        size,
        convert_to::vulkan(usage),
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
CreateImageHandle(vulkan::device const &device, std::uint32_t width, std::uint32_t height, std::uint32_t mip_levels,
                  std::uint32_t samples_count, graphics::FORMAT format, graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usage) noexcept
{
    VkImageCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr, 0,
        VK_IMAGE_TYPE_2D,
        convert_to::vulkan(format),
        { width, height, 1 },
        mip_levels,
        1,
        convert_to::vulkan(samples_count),
        convert_to::vulkan(tiling),
        convert_to::vulkan(usage),
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    std::optional<VkImage> image;

    VkImage handle;

    if (auto result = vkCreateImage(device.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
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

    auto vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device.handle(), "vkDebugMarkerSetObjectNameEXT");

    if (auto result = vkDebugMarkerSetObjectNameEXT(device.handle(), &info); result != VK_SUCCESS)
        throw std::runtime_error("failed to set the image debug marker: "s + std::to_string(result));
#endif

    return image;
}
}

std::shared_ptr<resource::image>
ResourceManager::CreateImage(graphics::FORMAT format, std::uint16_t width, std::uint16_t height, std::uint32_t mip_levels,
                             std::uint32_t samples_count, graphics::IMAGE_TILING tiling, graphics::IMAGE_USAGE usageFlags, VkMemoryPropertyFlags propertyFlags)
{
    std::shared_ptr<resource::image> image;

    auto handle = CreateImageHandle(device_, width, height, mip_levels, samples_count, format, tiling, usageFlags);

    if (handle) {
        auto const linearMemory = tiling == graphics::IMAGE_TILING::LINEAR;

        auto memory = memory_manager_.AllocateMemory(*handle, propertyFlags, linearMemory);

        if (memory) {
            if (auto result = vkBindImageMemory(device_.handle(), *handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
                std::cerr << "failed to bind image buffer memory: "s << result << '\n';

            else image.reset(
                new resource::image{memory, *handle, format, tiling, mip_levels, { width, height }},
                [this] (resource::image *ptr_image)
                {
                    ReleaseResource(*ptr_image);

                    delete ptr_image;
                }
            );
        }
    }

    return image;
}

std::shared_ptr<resource::image_view>
ResourceManager::CreateImageView(std::shared_ptr<resource::image> image, graphics::IMAGE_VIEW_TYPE view_type, VkImageAspectFlags aspectFlags) noexcept
{
    std::shared_ptr<resource::image_view> image_view;

    VkImageViewCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr, 0,
        image->handle(),
        convert_to::vulkan(view_type),
        convert_to::vulkan(image->format()),
        { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        { aspectFlags, 0, image->mip_levels(), 0, 1 }
    };

    VkImageView handle;

    if (auto result = vkCreateImageView(device_.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create image view: "s << result << '\n';

    else image_view.reset(new resource::image_view{handle, image, view_type}, [this] (resource::image_view *ptr_image_view) {
        ReleaseResource(*ptr_image_view);

        delete ptr_image_view;
    });

    return image_view;
}

std::shared_ptr<resource::sampler>
ResourceManager::CreateImageSampler(std::uint32_t mip_levels) noexcept
{
    std::shared_ptr<resource::sampler> sampler;

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
        0.f, static_cast<float>(mip_levels),
        VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        VK_FALSE
    };

    VkSampler handle;

    if (auto result = vkCreateSampler(device_.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
        std::cerr << "failed to create sampler: "s << result << '\n';

    else sampler.reset(
        new resource::sampler{handle},
        [this] (resource::sampler *ptr_sampler)
        {
            ReleaseResource(*ptr_sampler);

            delete ptr_sampler;
        }
    );

    return sampler;
}


std::shared_ptr<resource::buffer>
ResourceManager::CreateBuffer(VkDeviceSize size, graphics::BUFFER_USAGE usage, VkMemoryPropertyFlags properties) noexcept
{
    std::shared_ptr<resource::buffer> buffer;

    auto handle = CreateBufferHandle(device_, size, usage);

    if (handle) {
        auto const linearMemory = true;

        auto memory = memory_manager_.AllocateMemory(*handle, properties, linearMemory);

        if (memory) {
            if (auto result = vkBindBufferMemory(device_.handle(), *handle, memory->handle(), memory->offset()); result != VK_SUCCESS)
                std::cerr << "failed to bind buffer memory: "s << result << '\n';

            else buffer.reset(
                new resource::buffer{memory, *handle},
                [this] (resource::buffer *ptr_buffer)
                {
                    ReleaseResource(*ptr_buffer);

                    delete ptr_buffer;
                }
            );
        }
    }

    return buffer;
}


template<class T> requires mpl::one_of<std::remove_cvref_t<T>, resource::image, resource::sampler, resource::image_view, resource::buffer, resource::semaphore>
void ResourceManager::ReleaseResource(T &&resource) noexcept
{
    using R = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<R, resource::image>)
    {
        vkDestroyImage(device_.handle(), resource.handle(), nullptr);
        resource.memory().reset();
    }

    else if constexpr (std::is_same_v<R, resource::sampler>)
    {
        vkDestroySampler(device_.handle(), resource.handle(), nullptr);
    }

    else if constexpr (std::is_same_v<R, resource::image_view>)
    {
        vkDestroyImageView(device_.handle(), resource.handle(), nullptr);
    }

    else if constexpr (std::is_same_v<R, resource::buffer>)
    {
        vkDestroyBuffer(device_.handle(), resource.handle(), nullptr);
        resource.memory().reset();
    }

    else if constexpr (std::is_same_v<R, resource::semaphore>)
    {
        vkDestroySemaphore(device_.handle(), resource.handle(), nullptr);
    }
}

std::shared_ptr<resource::vertex_buffer> ResourceManager::CreateVertexBuffer(graphics::vertex_layout const &layout, std::size_t sizeInBytes) noexcept
{
    if (vertexBuffers_.count(layout) == 0) {
        std::shared_ptr<resource::buffer> stagingBuffer;
        std::shared_ptr<resource::buffer> deviceBuffer;
        
        auto const capacityInBytes = sizeInBytes * kVertexBufferIncreaseValue;

        {
            auto constexpr usageFlags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            stagingBuffer = CreateBuffer(sizeInBytes, usageFlags, propertyFlags);

            if (!stagingBuffer) {
                std::cerr << "failed to create staging vertex buffer\n"s;
                return { };
            }
        }

        {
            auto constexpr usageFlags = graphics::BUFFER_USAGE::TRANSFER_DESTINATION | graphics::BUFFER_USAGE::VERTEX_BUFFER;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            deviceBuffer = CreateBuffer(capacityInBytes, usageFlags, propertyFlags);

            if (!deviceBuffer) {
                std::cerr << "failed to create device vertex buffer\n"s;
                return { };
            }
        }

        vertexBuffers_.emplace(layout, std::make_shared<resource::vertex_buffer>(deviceBuffer, stagingBuffer, capacityInBytes, layout));
    }

    auto &vertexBuffer = vertexBuffers_[layout];

    if (vertexBuffer->stagingBufferSizeInBytes_ < sizeInBytes) {
        auto constexpr usageFlags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        vertexBuffer->stagingBuffer_ = CreateBuffer(sizeInBytes, usageFlags, propertyFlags);

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
        auto memory = memory_manager_.AllocateMemory(bufferHandle, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#endif
        std::cerr << "not enough device memory for vertex buffer\n"s;
        return { };
    }

    return vertexBuffer;
}

void ResourceManager::StageVertexData(std::shared_ptr<resource::vertex_buffer> vertexBuffer, std::vector<std::byte> const &container) const
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

void ResourceManager::TransferStagedVertexData(VkCommandPool transferCommandPool, graphics::transfer_queue const &transfer_queue) const
{
    for (auto &&[layout, vertexBuffer] : vertexBuffers_) {
        auto &&stagingBuffer = vertexBuffer->stagingBuffer();
        auto &&deviceBuffer = vertexBuffer->deviceBuffer();

        auto copyRegions = std::array{VkBufferCopy{ 0, 0, stagingBuffer.memory()->size() }};

        CopyBufferToBuffer(device_, transfer_queue, stagingBuffer.handle(), deviceBuffer.handle(), std::move(copyRegions), transferCommandPool);
    }
}

std::shared_ptr<resource::vertex_buffer> ResourceManager::vertex_buffer(graphics::vertex_layout const &layout) const
{
    if (vertexBuffers_.count(layout) == 0)
        return { };

    return vertexBuffers_.at(layout);
}


std::shared_ptr<resource::buffer>
CreateUniformBuffer(ResourceManager &resource_manager, std::size_t size)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::UNIFORM_BUFFER;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return resource_manager.CreateBuffer(size, usageFlags, propertyFlags);
}

std::shared_ptr<resource::buffer>
CreateCoherentStorageBuffer(ResourceManager &resource_manager, std::size_t size)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::STORAGE_BUFFER;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return resource_manager.CreateBuffer(size, usageFlags, propertyFlags);
}

std::shared_ptr<resource::buffer>
CreateStorageBuffer(ResourceManager &resource_manager, std::size_t size)
{
    auto constexpr usageFlags = graphics::BUFFER_USAGE::STORAGE_BUFFER;
    auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    return resource_manager.CreateBuffer(size, usageFlags, propertyFlags);
}

std::shared_ptr<resource::semaphore> ResourceManager::create_semaphore()
{
    std::shared_ptr<resource::semaphore> semaphore;

    VkSemaphoreCreateInfo constexpr createInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr, 0
    };

    VkSemaphore handle;

    if (auto result = vkCreateSemaphore(device_.handle(), &createInfo, nullptr, &handle); result == VK_SUCCESS) {
        semaphore.reset(
            new resource::semaphore{ handle }, [this] (resource::semaphore *ptr_semaphore)
            {
                ReleaseResource(*ptr_semaphore);

                delete ptr_semaphore;
            }
        );
    }

    else std::cerr << "failed to create a semaphore: "s << result << '\n';

    return semaphore;
}
