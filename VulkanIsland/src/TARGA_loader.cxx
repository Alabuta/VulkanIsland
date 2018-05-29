
#include <istream>

#include "main.h"
#include "TARGA_loader.h"



template<std::size_t N>
std::optional<texel_buffer_t> instantiate_texel_buffer()
{
    return std::make_optional<std::vector<vec<N, std::uint8_t>>>();
}

std::optional<texel_buffer_t> instantiate_texel_buffer(std::uint8_t pixelDepth)
{
    switch (pixelDepth) {
        case 8:
            return instantiate_texel_buffer<1>();

        case 16:
            return instantiate_texel_buffer<2>();

        case 24:
            return instantiate_texel_buffer<3>();

        case 32:
            return instantiate_texel_buffer<4>();

        default:
            return std::nullopt;
    }
}

void LoadUncompressedImage(Image &image, std::ifstream &file)
{
    auto buffer = instantiate_texel_buffer(image.pixelDepth);

    if (!buffer) return;

    std::visit([&image, &file] (auto &&buffer)
    {
        auto const size = static_cast<std::size_t>(image.width * image.height);

        buffer.resize(size);

        file.read(reinterpret_cast<char *>(std::data(buffer)), size);
        buffer.shrink_to_fit();

        image.data = std::move(buffer);

    }, std::move(buffer.value()));

    switch (image.pixelDepth) {
        case 8:
            image.pixelLayout = ePIXEL_LAYOUT::nRED;
            break;

        case 16:
            image.pixelLayout = ePIXEL_LAYOUT::nRG;
            break;

        case 24:
            image.pixelLayout = ePIXEL_LAYOUT::nBGR;
            break;

        case 32:
            image.pixelLayout = ePIXEL_LAYOUT::nBGRA;
            break;

        default:
            image.pixelLayout = ePIXEL_LAYOUT::nINVALID;
    }
}

[[nodiscard]] std::optional<Image> LoadTARGA(std::string_view _name)
{
    auto current_path = fs::current_path();

    fs::path directory{"contents"s};
    fs::path name{std::data(_name)};

    if (!fs::exists(current_path / directory))
        directory = current_path / fs::path{"../../VulkanIsland"s} / directory;

    std::ifstream file((directory / name).native(), std::ios::binary);

    if (!file.is_open())
        return { };

    struct header_t {
        byte_t IDLength;
        byte_t colorMapType;
        byte_t imageType;
        std::array<byte_t, 5> colorMapSpec;
        std::array<byte_t, 10> imageSpec;
    } header;

    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    auto const hasColorMap = (header.colorMapType & 0x01) == 0x01;

    Image image;

    image.width = static_cast<std::int16_t>((header.imageSpec.at(5) << 8) + header.imageSpec.at(4));
    image.height = static_cast<std::int16_t>((header.imageSpec.at(7) << 8) + header.imageSpec.at(6));
    image.pixelDepth = header.imageSpec.at(8);

    [[maybe_unused]] auto const alphaDepth = header.imageSpec.at(9) & 0x07; // First three bits from last byte of image specification field.

    if (image.width * image.height * image.pixelDepth < 0)
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

        file.seekg(colorMapStart, std::ios::cur);

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
            LoadUncompressedImage(image, file);
            break;

        // Run-length encoded true-color image
        case 0x0A:
            //LoadCompressedTARGA(image, file);
            break;

        default:
            return { };
    }

    file.close();

    if (image.pixelLayout == ePIXEL_LAYOUT::nINVALID)
        return { };

    return image;
}