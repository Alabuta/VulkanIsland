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
    struct image_info final {
		graphics::FORMAT format{graphics::FORMAT::UNDEFINED};
        graphics::IMAGE_VIEW_TYPE view_type;

		std::int32_t width{0}, height{0};

        std::size_t size_bytes;

        static auto constexpr image_pixels_deleter{[] (stbi_uc *pixels) noexcept { stbi_image_free(pixels); }};

        std::unique_ptr<stbi_uc, decltype(image_info::image_pixels_deleter)> pixels_ptr;
	};

    template<class T>
    [[nodiscard]] std::shared_ptr<resource::buffer>
    stage_data(vulkan::device &device, resource::resource_manager &resource_manager, std::span<T> texels_data)
    {
        auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
        auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

        //auto staging_buffer = resource_manager->create_staging_buffer(size_bytes);

        auto buffer = resource_manager.create_buffer(texels_data.size_bytes(), usage_flags, property_flags, graphics::RESOURCE_SHARING_MODE::EXCLUSIVE);

        if (buffer) {
            void *data;

            auto &&memory = buffer->memory();

            if (auto result = vkMapMemory(device.handle(), memory->handle(), memory->offset(), memory->size(), 0, &data); result != VK_SUCCESS)
                throw vulkan::exception(fmt::format("failed to map staging buffer memory: {0:#x}"s, result));

            else {
                std::ranges::copy(texels_data, static_cast<T *>(data));

                vkUnmapMemory(device.handle(), buffer->memory()->handle());
            }
        }

        return buffer;
    }

    image_info load_texture_data(std::string_view name)
    {
        fs::path contents{"contents/textures"sv};

        if (!fs::exists(fs::current_path() / contents))
            contents = fs::current_path() / "../"sv / contents;

        auto path = (contents / name).native();

        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

        std::int32_t w, h, component_count;

        if (auto result = stbi_info(converter.to_bytes(path).c_str(), &w, &h, &component_count); result != 1)
            throw resource::exception(fmt::format("failed to load an image: {0:#x}"s, name));

        auto texels = stbi_load(converter.to_bytes(path).c_str(), &w, &h, &component_count, component_count == 3 ? 4 : 0);

        if (texels == nullptr)
            throw resource::exception(fmt::format("failed to load an image: {0:#x}"s, name));

        auto format = graphics::FORMAT::UNDEFINED;

        component_count = component_count == 3 ? 4 : component_count;
        switch (component_count)
        {
            case 1:
                format = graphics::FORMAT::R8_SRGB;
                break;

            case 2:
                format = graphics::FORMAT::RG8_SRGB;
                break;

            case 3:
            case 4:
                format = graphics::FORMAT::RGBA8_SRGB;
                break;

            default:
                break;
        }

        image_info info{
            format,
            graphics::IMAGE_VIEW_TYPE::TYPE_2D,
            w, h,
            static_cast<std::size_t>(w * h * component_count * sizeof(std::byte)),
            std::unique_ptr<stbi_uc, decltype(image_info::image_pixels_deleter)>{texels, image_info::image_pixels_deleter}
        };

        return info;
    }

    [[nodiscard]]
    std::shared_ptr<resource::texture>
    create_and_stage_texture(vulkan::device &device, resource::resource_manager &resource_manager, std::string_view name, VkCommandPool transfer_command_pool)
    {
        auto const info = load_texture_data(name);

        auto staging_buffer = stage_data(device, resource_manager, std::span{std::to_address(info.pixels_ptr), info.size_bytes});
        if (staging_buffer == nullptr)
            return { };

        auto constexpr has_mip_maps = true;
        
        auto const width = static_cast<std::uint32_t>(info.width);
        auto const height = static_cast<std::uint32_t>(info.height);

        auto constexpr usage_flags = graphics::IMAGE_USAGE::TRANSFER_SOURCE | graphics::IMAGE_USAGE::TRANSFER_DESTINATION | graphics::IMAGE_USAGE::SAMPLED;
        auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;

        auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;

        auto type = graphics::IMAGE_TYPE::TYPE_2D;
        auto format = info.format;
        auto view_type = info.view_type;
        auto mip_levels = static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);
        auto aspect_flags = graphics::IMAGE_ASPECT::COLOR_BIT;
        auto samples_count = 1u;
        auto extent = renderer::extent{width, height};

        std::shared_ptr<resource::texture> texture;

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

        return texture;
    }
}

[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(vulkan::device &device, resource::resource_manager &resource_manager, std::string_view name, VkCommandPool transfer_command_pool)
{
    return create_and_stage_texture(device, resource_manager, name, transfer_command_pool);

#if 0
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
#endif
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
