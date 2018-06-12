
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

constexpr auto GetPixelLayout(std::uint8_t pixelDepth)
{
    switch (pixelDepth) {
        case 8:
            return ePIXEL_LAYOUT::nRED;
            break;

        case 16:
            return ePIXEL_LAYOUT::nRG;
            break;

        case 24:
            return ePIXEL_LAYOUT::nBGR;
            break;

        case 32:
            return ePIXEL_LAYOUT::nBGRA;
            break;

        default:
            return ePIXEL_LAYOUT::nINVALID;
    }
}

void LoadUncompressedTrueColorImage(Image &image, TARGA &targa, std::ifstream &file)
{
    texel_buffer_t buffer;

    if (auto result = instantiate_texel_buffer(targa.pixelDepth); result)
        buffer = result.value();

    else return;

    image.pixelLayout = GetPixelLayout(targa.pixelDepth);

    std::visit([&image, &file] (auto &&buffer)
    {
        buffer.resize(image.width * image.height);

        using texel_t = typename std::decay_t<decltype(buffer)>::value_type;

        file.read(reinterpret_cast<char *>(std::data(buffer)), std::size(buffer) * sizeof(texel_t));

        if constexpr (texel_t::size == 3)
        {
            using vec_type = vec<4, typename texel_t::value_type>;

            std::vector<vec_type> intermidiateBuffer(std::size(buffer));

            std::size_t i = 0;
            for (auto &&texel : buffer)
                intermidiateBuffer.at(++i - 1) = std::move(vec_type{texel.array.at(0), texel.array.at(1), texel.array.at(2), 1});

            image.data = std::move(intermidiateBuffer);
        }

        else image.data = std::move(buffer);

    }, std::move(buffer));
}

void LoadUncompressedColorMappedImage(Image &image, TARGA &targa, std::ifstream &file)
{
    if ((targa.header.colorMapType & 0x01) != 0x01)
        return;

    targa.colorMapDepth = targa.header.colorMapSpec.at(4);

    texel_buffer_t palette;

    if (auto buffer = instantiate_texel_buffer(targa.colorMapDepth); buffer)
        palette = buffer.value();

    else return;

    image.pixelLayout = GetPixelLayout(targa.colorMapDepth);

    auto colorMapStart = static_cast<std::size_t>((targa.header.colorMapSpec.at(1) << 8) + targa.header.colorMapSpec.at(0));
    auto colorMapLength = static_cast<std::size_t>((targa.header.colorMapSpec.at(3) << 8) + targa.header.colorMapSpec.at(2));

    file.seekg(colorMapStart, std::ios::cur);

    palette = std::visit([&] (auto &&palette) -> texel_buffer_t
    {
        palette.resize(colorMapLength);

        using texel_t = typename std::decay_t<decltype(palette)>::value_type;

        file.read(reinterpret_cast<char *>(std::data(palette)), std::size(palette) * sizeof(texel_t));

        if constexpr (texel_t::size == 3)
        {
            using vec_type = vec<4, typename texel_t::value_type>;

            std::vector<vec_type> intermidiateBuffer(std::size(palette));

            std::size_t i = 0;
            for (auto &&texel : palette)
                intermidiateBuffer.at(++i - 1) = std::move(vec_type{texel.array.at(0), texel.array.at(1), texel.array.at(2), 1});

            return std::move(intermidiateBuffer);
        }

        else return std::move(palette);

    }, std::move(palette));

    std::visit([&image, &file] (auto &&palette)
    {
        std::vector<std::size_t> indices(image.width * image.height);
        file.read(reinterpret_cast<char *>(std::data(indices)), std::size(indices) * sizeof(std::byte));

        std::decay_t<decltype(palette)> buffer(image.width * image.height);

        std::ptrdiff_t begin, end;
        std::size_t colorIndex;

        for (auto it_index = std::cbegin(indices); it_index < std::cend(indices); ) {
            begin = std::distance(std::cbegin(indices), it_index);

            colorIndex = *it_index;

            it_index = std::partition_point(it_index, std::cend(indices), [colorIndex] (auto index) { return index == colorIndex; });

            end = std::distance(std::cbegin(indices), it_index);

            std::fill_n(std::next(std::begin(buffer), begin), end - begin, palette.at(colorIndex));
        }

        image.data = std::move(buffer);

    }, std::move(palette));
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

    TARGA targa;

    file.read(reinterpret_cast<char *>(&targa.header), sizeof(targa.header));

    targa.width = static_cast<std::int16_t>((targa.header.imageSpec.at(5) << 8) + targa.header.imageSpec.at(4));
    targa.height = static_cast<std::int16_t>((targa.header.imageSpec.at(7) << 8) + targa.header.imageSpec.at(6));
    targa.pixelDepth = targa.header.imageSpec.at(8);

    Image image;

    image.width = targa.width;
    image.height = targa.height;
    image.pixelDepth = targa.pixelDepth;

    [[maybe_unused]] auto const alphaDepth = targa.header.imageSpec.at(9) & 0x07; // First three bits from last byte of image specification field.

    if (image.width * image.height * image.pixelDepth < 0)
        return { };

    std::vector<std::byte> imageID(targa.header.IDLength);

    if (std::size(imageID))
        file.read(reinterpret_cast<char *>(std::data(imageID)), sizeof(std::size(imageID)));

    switch (targa.header.imageType) {
        // No image data is present
        case 0x00:
        // Uncompressed monochrome image
        case 0x03:
        // Run-length encoded color-mapped image
        case 0x09:
        // Run-length encoded monochrome image
        case 0x0B:
        // Run-length encoded true-color image
        case 0x0A:
            return { };

        // Uncompressed color-mapped image
        case 0x01:
            std::cout << measure<>::execution(LoadUncompressedColorMappedImage, image, targa, file) << '\n';
            break;

        // Uncompressed true-color image
        case 0x02:
            std::cout << measure<>::execution(LoadUncompressedTrueColorImage, image, targa, file) << '\n';
            break;

        default:
            return { };
    }

    file.close();

    if (image.pixelLayout == ePIXEL_LAYOUT::nINVALID)
        return { };

    return image;
}