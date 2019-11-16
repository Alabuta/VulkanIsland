#include <cmath>

#include "resources/buffer.hxx"
#include "resources/image.hxx"
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
        std::shared_ptr<resource::buffer> staging_buffer;
        std::shared_ptr<resource::buffer> device_buffer;
        
        auto const capacityInBytes = sizeInBytes * kVertexBufferIncreaseValue;

        {
            auto constexpr usageFlags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            staging_buffer = CreateBuffer(sizeInBytes, usageFlags, propertyFlags);

            if (!staging_buffer) {
                std::cerr << "failed to create staging vertex buffer\n"s;
                return { };
            }
        }

        {
            auto constexpr usageFlags = graphics::BUFFER_USAGE::TRANSFER_DESTINATION | graphics::BUFFER_USAGE::VERTEX_BUFFER;
            auto constexpr propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            device_buffer = CreateBuffer(capacityInBytes, usageFlags, propertyFlags);

            if (!device_buffer) {
                std::cerr << "failed to create device vertex buffer\n"s;
                return { };
            }
        }

        vertexBuffers_.emplace(layout, std::make_shared<resource::vertex_buffer>(device_buffer, staging_buffer, capacityInBytes, layout));
    }

    auto &vertexBuffer = vertexBuffers_[layout];

    if (vertexBuffer->staging_buffer_size_in_bytes_ < sizeInBytes) {
        auto constexpr usageFlags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
        auto constexpr propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        vertexBuffer->staging_buffer_ = CreateBuffer(sizeInBytes, usageFlags, propertyFlags);

        if (!vertexBuffer->staging_buffer_) {
            std::cerr << "failed to extend staging vertex buffer\n"s;
            return { };
        }

        vertexBuffer->staging_buffer_size_in_bytes_ = sizeInBytes;
    }

    if (vertexBuffer->available_memory_size() < sizeInBytes) {
        // TODO: sparse memory binding
#if NOT_YET_IMPLEMENTED
        auto bufferHandle = vertexBuffer->device_buffer_->handle();
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
        if (vertexBuffer->available_memory_size() < lengthInBytes)
            throw std::runtime_error("not enough device memory for vertex buffer"s);

        auto &&memory = vertexBuffer->staging_buffer().memory();

        void *ptr;

        if (auto result = vkMapMemory(device_.handle(), memory->handle(), vertexBuffer->staging_buffer_offset(), lengthInBytes, 0, &ptr); result != VK_SUCCESS)
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
        auto &&staging_buffer = vertexBuffer->staging_buffer();
        auto &&device_buffer = vertexBuffer->device_buffer();

        auto copyRegions = std::array{VkBufferCopy{ 0, 0, staging_buffer.memory()->size() }};

        CopyBufferToBuffer(device_, transfer_queue, staging_buffer.handle(), device_buffer.handle(), std::move(copyRegions), transferCommandPool);
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
