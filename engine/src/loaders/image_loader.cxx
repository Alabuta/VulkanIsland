#include <ranges>

#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "resources/resource_manager.hxx"

#include "renderer/command_buffer.hxx"

#include "loaders/TARGA_loader.hxx"

#include "image_loader.hxx"


namespace
{
    template<class T>
    [[nodiscard]] std::shared_ptr<resource::buffer>
    stage_data(vulkan::device &device, resource::resource_manager &resource_manager, std::span<T> const container)
    {
        auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
        auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

        auto buffer = resource_manager.create_buffer(container.size_bytes(), usage_flags, property_flags, graphics::RESOURCE_SHARING_MODE::EXCLUSIVE);

        if (buffer) {
            void *data;

            auto &&memory = buffer->memory();

            if (auto result = vkMapMemory(device.handle(), memory->handle(), memory->offset(), memory->size(), 0, &data); result != VK_SUCCESS)
                throw vulkan::exception(fmt::format("failed to map staging buffer memory: {0:#x}"s, result));

            else {
                std::ranges::copy(container, static_cast<T *>(data));

                vkUnmapMemory(device.handle(), buffer->memory()->handle());
            }
        }

        return buffer;
    }
}

[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(vulkan::device &device, resource::resource_manager &resource_manager, std::string_view name, VkCommandPool transfer_command_pool)
{
    std::shared_ptr<resource::texture> texture;

    auto constexpr generateMipMaps = true;

    if (auto raw_image = LoadTARGA(name); raw_image) {
        auto staging_buffer = std::visit([&device, &resource_manager] (auto &&data)
        {
            return stage_data(device, resource_manager, std::span{data});
        }, std::move(raw_image->data));

        if (staging_buffer) {
            auto const width = static_cast<std::uint16_t>(raw_image->width);
            auto const height = static_cast<std::uint16_t>(raw_image->height);

            auto constexpr usage_flags = graphics::IMAGE_USAGE::TRANSFER_SOURCE | graphics::IMAGE_USAGE::TRANSFER_DESTINATION | graphics::IMAGE_USAGE::SAMPLED;
            auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;

            auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;

            auto type = graphics::IMAGE_TYPE::TYPE_2D;
            auto format = raw_image->format;
            auto view_type = raw_image->view_type;
            auto mip_levels = raw_image->mip_levels;
            auto aspect_flags = graphics::IMAGE_ASPECT::COLOR_BIT;
            auto samples_count = 1u;

            auto extent = renderer::extent{width, height};

            if (auto image = resource_manager.create_image(type, format, extent, mip_levels, samples_count, tiling, usage_flags, property_flags); image) {
                if (auto view = resource_manager.create_image_view(image, view_type, aspect_flags); view)
            #if NOT_YET_IMPLEMENTED
                    if (auto sampler = resource_manager.create_image_sampler(mip_levels()); sampler)
                        texture.emplace(image, *view, sampler);
            #else
                    texture = std::make_shared<resource::texture>(image, view, nullptr);
            #endif
            }

            if (texture) {
                image_layout_transition(device, device.transfer_queue, *texture->image, graphics::IMAGE_LAYOUT::UNDEFINED,
                                      graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION, transfer_command_pool);

                copy_buffer_to_image(device, device.transfer_queue, staging_buffer->handle(), texture->image->handle(), extent, transfer_command_pool);

                if (generateMipMaps)
                    generate_mip_maps(device, device.transfer_queue, *texture->image, transfer_command_pool);

                else image_layout_transition(device, device.transfer_queue, *texture->image, graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION,
                                           graphics::IMAGE_LAYOUT::SHADER_READ_ONLY, transfer_command_pool);
            }
        }
    }

    else throw resource::exception(fmt::format("failed to load an image: {0:#x}"s, name));


    return texture;
}
