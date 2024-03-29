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
#include <stb/stb_image.h>

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

    image_info load_texture_data(std::string_view name)
    {
        fs::path contents{"contents/textures"sv};

        if (!fs::exists(fs::current_path() / contents))
            contents = fs::current_path() / "../"sv / contents;

        auto path = (contents / name).native();

        /*std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        const std::wstring_convert::byte_string &p = converter.to_bytes(path);*/
        std::string p{std::begin(path), std::end(path)};

        std::int32_t w, h, component_count;

        if (auto result = stbi_info(p.c_str(), &w, &h, &component_count); result != 1)
            throw resource::exception(fmt::format("failed to load an image: {}", name));

        auto texels = stbi_load(p.c_str(), &w, &h, &component_count, component_count == 3 ? 4 : 0);

        if (texels == nullptr)
            throw resource::exception(fmt::format("failed to load an image: {}", name));

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
            static_cast<std::size_t>(w * h * component_count) * sizeof(std::byte),
            std::unique_ptr<stbi_uc, decltype(image_info::image_pixels_deleter)>{texels, image_info::image_pixels_deleter}
        };

        return info;
    }

    template<class T>
    [[nodiscard]] std::shared_ptr<resource::buffer>
    stage_data(vulkan::device &device, resource::resource_manager &resource_manager, std::span<T> texels_data)
    {
        auto constexpr usage_flags = graphics::BUFFER_USAGE::TRANSFER_SOURCE;
        auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::HOST_VISIBLE | graphics::MEMORY_PROPERTY_TYPE::HOST_COHERENT;

        //auto staging_buffer = resource_manager->create_staging_buffer(size_bytes);

        auto buffer = resource_manager.create_buffer(texels_data.size_bytes(), usage_flags, property_flags, graphics::RESOURCE_SHARING_MODE::EXCLUSIVE);

        if (buffer) {
            void *data = nullptr;

            auto &&memory = buffer->memory();

            if (auto result = vkMapMemory(device.handle(), memory->handle(), memory->offset(), memory->size(), 0, &data); result != VK_SUCCESS)
                throw vulkan::exception(fmt::format("failed to map staging buffer memory: {0:#x}", result));

            else {
                std::ranges::copy(texels_data, static_cast<T *>(data));

                vkUnmapMemory(device.handle(), buffer->memory()->handle());
            }
        }

        return buffer;
    }
}

graphics::IMAGE_USAGE get_texture_usage_flags(bool generate_mipmaps)
{
    if (!generate_mipmaps)
        return graphics::IMAGE_USAGE::TRANSFER_DESTINATION | graphics::IMAGE_USAGE::SAMPLED;

    return graphics::IMAGE_USAGE::TRANSFER_SOURCE | graphics::IMAGE_USAGE::TRANSFER_DESTINATION | graphics::IMAGE_USAGE::SAMPLED;
}

[[nodiscard]]
std::shared_ptr<resource::texture>
load_texture(vulkan::device &device, render::config const &config, resource::resource_manager &resource_manager, std::string_view name, VkCommandPool transfer_command_pool)
{
    auto const info = load_texture_data(name);

    auto staging_buffer = stage_data(device, resource_manager, std::span{std::to_address(info.pixels_ptr), info.size_bytes});
    if (staging_buffer == nullptr)
        return { };

    auto const generate_mipmaps = config.generate_mipmaps;

    auto const width = static_cast<std::uint32_t>(info.width);
    auto const height = static_cast<std::uint32_t>(info.height);

    auto const usage_flags = get_texture_usage_flags(generate_mipmaps);
    auto constexpr property_flags = graphics::MEMORY_PROPERTY_TYPE::DEVICE_LOCAL;

    auto constexpr tiling = graphics::IMAGE_TILING::OPTIMAL;

    auto type = graphics::IMAGE_TYPE::TYPE_2D;
    auto format = info.format;
    auto view_type = info.view_type;
    auto mip_levels = generate_mipmaps ? static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height))) + 1) : 1;
    auto aspect_flags = graphics::IMAGE_ASPECT::COLOR_BIT;
    auto samples_count = 1u;
    auto extent = render::extent{ width, height};

    std::shared_ptr<resource::texture> texture;

    if (auto image = resource_manager.create_image(type, format, extent, mip_levels, samples_count, tiling, usage_flags, property_flags); image) {
        if (auto view = resource_manager.create_image_view(image, view_type, aspect_flags); view)
    #ifdef NOT_YET_IMPLEMENTED
            if (auto sampler = resource_manager.create_image_sampler(mip_levels()); sampler)
                texture.emplace(image, *view, sampler);
    #else
            texture = std::make_shared<resource::texture>(image, view, nullptr);
    #endif
    }

    if (texture) {
        image_layout_transition(
                device,
                device.transfer_queue,
                *texture->image,
                graphics::IMAGE_LAYOUT::UNDEFINED,
                graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION,
                transfer_command_pool);

        copy_buffer_to_image(device, device.transfer_queue, staging_buffer->handle(), texture->image->handle(), extent, transfer_command_pool);

        if (generate_mipmaps)
            generate_mip_maps(device, device.transfer_queue, *texture->image, transfer_command_pool);

        else
            image_layout_transition(
                    device,
                    device.transfer_queue,
                    *texture->image,
                    graphics::IMAGE_LAYOUT::TRANSFER_DESTINATION,
                    graphics::IMAGE_LAYOUT::SHADER_READ_ONLY,
                    transfer_command_pool);
    }

    return texture;
}
