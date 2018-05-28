
#include <istream>

#include "main.h"
#include "TARGA_loader.h"

enum class ePIXEL_LAYOUT {
    nRED = 0, nRG, nRGB, nBGR, nRGBA, nBGRA
};

bool LoadUncompressedTARGA(Image &_image, std::ifstream &_file, std::vector<std::byte> &pixels)
{
    auto const count = static_cast<std::size_t>(_image.bpp * _image.width * _image.height);

    ePIXEL_LAYOUT pixelLayout;

    switch (_image.bpp) {
        case 1:
            pixelLayout = ePIXEL_LAYOUT::nRED;
            break;

        case 2:
            pixelLayout = ePIXEL_LAYOUT::nRG;
            break;

        case 3:
            pixelLayout = ePIXEL_LAYOUT::nBGR;
            break;

        case 4:
            pixelLayout = ePIXEL_LAYOUT::nBGRA;
            break;

        default:
            return false;
    }

    pixels.resize(count);
    _file.read(reinterpret_cast<char *>(std::data(pixels)), count);
    pixels.shrink_to_fit();

    return true;
}

[[nodiscard]] std::optional<std::pair<std::int32_t, std::int32_t>> LoadTARGA(std::string_view _name, std::vector<std::byte> &pixels)
{
    auto current_path = fs::current_path();

    fs::path directory{"contents"s};
    fs::path name{std::data(_name)};

    if (!fs::exists(current_path / directory))
        directory = current_path / fs::path{"../../VulkanIsland"s} / directory;

    std::ifstream file((directory / name).native(), std::ios::binary);

    if (!file.is_open())
        return { };

#if 0
    struct header_t {
        std::byte IDLength;
        std::byte colorMapType;
        std::byte imageType;
        std::array<std::byte, 5> colorMapSpec;
        std::array<std::byte, 10> imageSpec;
    } header;

    file.read(reinterpret_cast<char *>(&header), sizeof(header_t));

    struct image_and_color_map_data_t {
        std::vector<std::byte> image_id;
        std::vector<std::byte> color_map_data;
        std::vector<std::byte> image_data;
    } image_and_color_map_data;

    file.read(reinterpret_cast<char *>(&image_and_color_map_data), sizeof(image_and_color_map_data_t));

    image_and_color_map_data.image_id.resize(static_cast<std::size_t>(header.IDLength));
    file.read(reinterpret_cast<char *>(std::data(image_and_color_map_data.image_id)), sizeof(std::byte) * static_cast<std::size_t>(header.IDLength));
#endif

    using byte_t = std::int8_t;

    struct header_t {
        byte_t IDLength;
        byte_t colorMapType;
        byte_t imageType;
        std::array<byte_t, 5> colorMapSpec;
        std::array<byte_t, 10> imageSpec;
    } header;

    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    auto const hasColorMap = (header.colorMapType & 0x01) == 0x01;

    auto const width = static_cast<std::int16_t>((header.imageSpec.at(5) << 8) + header.imageSpec.at(4));
    auto const height = static_cast<std::int16_t>((header.imageSpec.at(7) << 8) + header.imageSpec.at(6));

    auto const pixelDepth = header.imageSpec.at(8);
    auto const alphaDepth = header.imageSpec.at(9) & 0x07; // First three bits from last byte of image specification field.

    Image image;

    image.width = width;
    image.height = height;
    image.bpp = pixelDepth / 8;

    if (image.width * image.height * image.bpp < 0)
        return { };

    std::vector<std::byte> imageID(header.IDLength);

    if (std::size(imageID))
        file.read(reinterpret_cast<char *>(std::data(imageID)), sizeof(std::size(imageID)));

    std::vector<std::byte> colorMap;

    if (hasColorMap) {
        auto colorMapStart = static_cast<std::size_t>((header.colorMapSpec.at(1) << 8) + header.colorMapSpec.at(0));
        auto colorMapLength = static_cast<std::size_t>((header.colorMapSpec.at(3) << 8) + header.colorMapSpec.at(2));
        auto colorMapDepth = header.colorMapSpec.at(4);

        colorMap.resize(colorMapLength * colorMapDepth / 8);

        file.read(reinterpret_cast<char *>(std::data(colorMap)), sizeof(std::size(colorMap)));
    }

    switch (header.imageType) {
        // No image data is present
        case 0x00:
        // Uncompressed color-mapped image
        case 0x01:
        // Uncompressed monochrome image
        case 0x03:
        // Run-length encoded color-mapped image
        case 0x09:
        // Run-length encoded monochrome image
        case 0x0B:
            return { };

        // Uncompressed true-color image
        case 0x02:
            LoadUncompressedTARGA(image, file, pixels);
            break;

        // Run-length encoded true-color image
        case 0x0A:
            //LoadCompressedTARGA(image, file);
            break;

        default:
            return { };
    }

    file.close();

    return std::make_pair(image.width, image.height);
}