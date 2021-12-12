#include <ranges>
#include <locale>
#include <codecvt>

#include <string>
using namespace std::string_literals;
using namespace std::string_view_literals;

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_BMP
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM 
#include <stb_image.h>

#include "resources/buffer.hxx"
#include "resources/image.hxx"
#include "resources/resource_manager.hxx"

#include "renderer/command_buffer.hxx"

#include "loaders/TARGA_loader.hxx"

#include "image_loader.hxx"


namespace
{
    [[nodiscard]] std::shared_ptr<resource::buffer>
    stage_data(vulkan::device &device, resource::resource_manager &resource_manager, std::size_t size_bytes, std::span<std::byte> texels_data)
    {
        auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
        auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

        /*auto staging_buffer = resource_manager->create_staging_buffer(size_bytes);
        auto buffer = resource_manager->stage_vertex_data(vertex_layout, vertex_staging_buffer, app.transfer_command_pool);*/

        auto buffer = resource_manager.create_buffer(texels_data.size_bytes(), usage_flags, property_flags, graphics::RESOURCE_SHARING_MODE::EXCLUSIVE);

        if (buffer) {
            void *data;

            auto &&memory = buffer->memory();

            if (auto result = vkMapMemory(device.handle(), memory->handle(), memory->offset(), memory->size(), 0, &data); result != VK_SUCCESS)
                throw vulkan::exception(fmt::format("failed to map staging buffer memory: {0:#x}"s, result));

            else {
                std::ranges::copy(texels_data, static_cast<std::byte *>(data));

                vkUnmapMemory(device.handle(), buffer->memory()->handle());
            }
        }

        return buffer;
    }

    struct image_info final {
		graphics::FORMAT format{graphics::FORMAT::UNDEFINED};
        graphics::IMAGE_VIEW_TYPE view_type;

		std::int32_t width{0}, height{0};
		std::int32_t mip_levels{1};
	};

    image_info get_texture_info(std::string_view name/*, std::span<std::byte> vertex_buffer*/)
    {
        fs::path contents{"contents/textures"sv};

        if (!fs::exists(fs::current_path() / contents))
            contents = fs::current_path() / "../"sv / contents;

        auto path = (contents / name).native();

        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

        std::int32_t w, h, n;
        if (auto result = stbi_info(converter.to_bytes(path).c_str(), &w, &h, &n); result != 0)
            throw resource::exception(fmt::format("failed to load an image: {0:#x}"s, name));

        auto format = graphics::FORMAT::UNDEFINED;

        switch (n)
        {
            case 1:
                format = graphics::FORMAT::R8_SRGB;
                break;

            case 2:
                format = graphics::FORMAT::RG8_SRGB;
                break;

            case 3:
                format = graphics::FORMAT::RGB8_SRGB;
                break;

            case 4:
                format = graphics::FORMAT::RGBA8_SRGB;
                break;

            default:
                break;
        }

        return image_info{
            graphics::FORMAT::UNDEFINED,
            graphics::IMAGE_VIEW_TYPE::TYPE_2D,
            w, h,
            1
        };
    }

    stbi_uc *load_texture_data(std::string_view name)
    {
        fs::path contents{"contents/textures"sv};

        if (!fs::exists(fs::current_path() / contents))
            contents = fs::current_path() / "../"sv / contents;

        auto path = (contents / name).native();

        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

        std::int32_t w, h, n;
        auto texels = stbi_load(converter.to_bytes(path).c_str(), &w, &h, &n, 0);

        if (texels == nullptr)
            throw resource::exception(fmt::format("failed to load an image: {0:#x}"s, name));

        //auto size_bytes = static_cast<VkDeviceSize>(w * h * n * sizeof(std::byte));

        return texels;
    }

    [[nodiscard]]
    std::shared_ptr<resource::texture> create_and_stage_texture(image_info const &info, std::span<std::byte> vertex_buffer)
    {
        std::shared_ptr<resource::texture> texture;

        auto const has_mip_maps = info.mip_levels > 1;


        return texture;
    }

}

[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(vulkan::device &device, resource::resource_manager &resource_manager, std::string_view name, VkCommandPool transfer_command_pool)
{
    load_texture(name);
    std::shared_ptr<resource::texture> texture;

    auto constexpr has_mip_maps = true;

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

                if (has_mip_maps)
                    generate_mip_maps(device, device.transfer_queue, *texture->image, transfer_command_pool);

                else image_layout_transition(device, device.transfer_queue, *texture->image, graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION,
                                           graphics::IMAGE_LAYOUT::SHADER_READ_ONLY, transfer_command_pool);
            }
        }
    }

    else throw resource::exception(fmt::format("failed to load an image: {0:#x}"s, name));


    return texture;
}

#if 0
[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(vulkan::device &device, resource::resource_manager &resource_manager, std::string_view name, VkCommandPool transfer_command_pool)
{
    std::shared_ptr<resource::texture> texture;

    auto constexpr has_mip_maps = true;

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

                if (has_mip_maps)
                    generate_mip_maps(device, device.transfer_queue, *texture->image, transfer_command_pool);

                else image_layout_transition(device, device.transfer_queue, *texture->image, graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION,
                                           graphics::IMAGE_LAYOUT::SHADER_READ_ONLY, transfer_command_pool);
            }
        }
    }

    else throw resource::exception(fmt::format("failed to load an image: {0:#x}"s, name));


    return texture;
}
#endif
