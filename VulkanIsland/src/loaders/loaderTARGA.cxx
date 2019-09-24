
#include <istream>

#include "main.hxx"

#include "loaderTARGA.hxx"



template<std::size_t N>
std::optional<texel_buffer_t> instantiate_texel_buffer()
{
    return std::make_optional<std::vector<math::vec<N, std::uint8_t>>>();
}

std::optional<texel_buffer_t> instantiate_texel_buffer(std::uint8_t pixelDepth) noexcept
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

constexpr PIXEL_LAYOUT GetPixelLayout(std::uint8_t pixelDepth) noexcept
{
    switch (pixelDepth) {
        case 8:
            return PIXEL_LAYOUT::nRED;
            break;

        case 16:
            return PIXEL_LAYOUT::nRG;
            break;

        case 24:
            return PIXEL_LAYOUT::nBGR;
            break;

        case 32:
            return PIXEL_LAYOUT::nBGRA;
            break;

        default:
            return PIXEL_LAYOUT::nUNDEFINED;
    }
}

constexpr graphics::FORMAT GetPixelFormat(PIXEL_LAYOUT pixelLayout) noexcept
{
    switch (pixelLayout) {
        case PIXEL_LAYOUT::nRED:
            return graphics::FORMAT::R8_UNORM;

        case PIXEL_LAYOUT::nRG:
            return graphics::FORMAT::RG8_UNORM;

        case PIXEL_LAYOUT::nRGB:
            return graphics::FORMAT::RGB8_UNORM;

        case PIXEL_LAYOUT::nBGR:
            return graphics::FORMAT::BGR8_UNORM;

        case PIXEL_LAYOUT::nRGBA:
            return graphics::FORMAT::RGBA8_UNORM;

        case PIXEL_LAYOUT::nBGRA:
            return graphics::FORMAT::BGRA8_UNORM;

        case PIXEL_LAYOUT::nUNDEFINED:
        default:
            return graphics::FORMAT::UNDEFINED;
    }
}


void LoadUncompressedTrueColorImage(TARGA &targa, std::ifstream &file)
{
    texel_buffer_t buffer;

    if (auto result = instantiate_texel_buffer(targa.pixelDepth); result)
        buffer = result.value();

    else return;

    targa.pixelLayout = GetPixelLayout(targa.pixelDepth);

    std::visit([&targa, &file] (auto &&buffer)
    {
        buffer.resize(static_cast<std::size_t>(targa.width * targa.height));

        using texel_type = typename std::remove_cvref_t<decltype(buffer)>::value_type;

        file.read(reinterpret_cast<char *>(std::data(buffer)), static_cast<std::int64_t>(std::size(buffer) * sizeof(texel_type)));

        if constexpr (texel_type::size == 3) {
            targa.pixelLayout = targa.pixelLayout == PIXEL_LAYOUT::nRGB ? PIXEL_LAYOUT::nRGBA : PIXEL_LAYOUT::nBGRA;

            using vec_type = math::vec<4, typename texel_type::value_type>;

            std::vector<vec_type> intermidiateBuffer(std::size(buffer));

            std::size_t i = 0;
            for (auto &&texel : buffer)
                intermidiateBuffer.at(++i - 1) = std::move(vec_type{texel.array.at(0), texel.array.at(1), texel.array.at(2), 1});

            targa.data = std::move(intermidiateBuffer);
        }

        else targa.data = std::move(buffer);

    }, std::move(buffer));
}

void LoadUncompressedColorMappedImage(TARGA &targa, std::ifstream &file)
{
    if ((targa.header.colorMapType & 0x01) != 0x01)
        return;

    targa.colorMapDepth = targa.header.colorMapSpec.at(4);

    texel_buffer_t palette;

    if (auto buffer = instantiate_texel_buffer(targa.colorMapDepth); buffer)
        palette = buffer.value();

    else return;

    targa.pixelLayout = GetPixelLayout(targa.colorMapDepth);

    using pos_type_t = std::remove_cvref_t<decltype(file)>::pos_type;

    auto colorMapStart = static_cast<pos_type_t>((targa.header.colorMapSpec.at(1) << 8) + targa.header.colorMapSpec.at(0));
    auto colorMapLength = static_cast<std::size_t>((targa.header.colorMapSpec.at(3) << 8) + targa.header.colorMapSpec.at(2));

    file.seekg(colorMapStart, std::ios::cur);

    palette = std::visit([&] (auto &&palette) -> texel_buffer_t
    {
        palette.resize(colorMapLength);

        using texel_type = typename std::remove_cvref_t<decltype(palette)>::value_type;

        file.read(reinterpret_cast<char *>(std::data(palette)), static_cast<std::streamsize>(std::size(palette) * sizeof(texel_type)));

        if constexpr (texel_type::size == 3) {
            targa.pixelLayout = targa.pixelLayout == PIXEL_LAYOUT::nRGB ? PIXEL_LAYOUT::nRGBA : PIXEL_LAYOUT::nBGRA;

            using vec_type = math::vec<4, typename texel_type::value_type>;

            std::vector<vec_type> intermidiateBuffer(std::size(palette));

            std::size_t i = 0;
            for (auto &&texel : palette)
                intermidiateBuffer.at(++i - 1) = std::move(vec_type{texel.array.at(0), texel.array.at(1), texel.array.at(2), 1});

            return intermidiateBuffer;
        }

        else return std::move(palette);

    }, std::move(palette));

    std::visit([&targa, &file] (auto &&palette)
    {
        std::vector<std::size_t> indices(static_cast<std::size_t>(targa.width * targa.height));
        file.read(reinterpret_cast<char *>(std::data(indices)), static_cast<std::streamsize>(std::size(indices) * sizeof(std::byte)));

        std::remove_cvref_t<decltype(palette)> buffer(static_cast<std::size_t>(targa.width * targa.height));

        std::ptrdiff_t begin, end;
        std::size_t colorIndex;

        for (auto it_index = std::cbegin(indices); it_index < std::cend(indices); ) {
            begin = std::distance(std::cbegin(indices), it_index);

            colorIndex = *it_index;

            it_index = std::partition_point(it_index, std::cend(indices), [colorIndex] (auto index) { return index == colorIndex; });

            end = std::distance(std::cbegin(indices), it_index);

            std::fill_n(std::next(std::begin(buffer), begin), end - begin, palette.at(colorIndex));
        }

        targa.data = std::move(buffer);

    }, std::move(palette));
}

[[nodiscard]] std::optional<RawImage> LoadTARGA(std::string_view name)
{
    fs::path contents{"contents/scenes"sv};

    if (!fs::exists(fs::current_path() / contents))
        contents = fs::current_path() / "../"sv / contents;

    auto path = contents / name;

    std::ifstream file{path.native(), std::ios::in | std::ios::binary};

    if (file.bad() || file.fail())
        return { };

    TARGA targa;

    file.read(reinterpret_cast<char *>(&targa.header), sizeof(targa.header));

    targa.width = static_cast<std::int16_t>((targa.header.imageSpec.at(5) << 8) + targa.header.imageSpec.at(4));
    targa.height = static_cast<std::int16_t>((targa.header.imageSpec.at(7) << 8) + targa.header.imageSpec.at(6));
    targa.pixelDepth = targa.header.imageSpec.at(8);

    if (targa.width * targa.height * targa.pixelDepth < 0)
        return { };

    // 0x07 - first three bits from last byte of image specification field.
    [[maybe_unused]] auto const alphaDepth = targa.header.imageSpec.at(9) & 0x07;

    std::vector<std::byte> imageID(targa.header.IDLength);

    if (std::size(imageID))
        file.read(reinterpret_cast<char *>(std::data(imageID)), sizeof(std::size(imageID)));

    switch (targa.header.imageType) {
        case 0x00:        // No image data is present
        case 0x03:        // Uncompressed monochrome image
        case 0x09:        // Run-length encoded color-mapped image
        case 0x0B:        // Run-length encoded monochrome image
        case 0x0A:        // Run-length encoded true-color image
            return { };

        case 0x01:        // Uncompressed color-mapped image
            LoadUncompressedColorMappedImage(targa, file);
            break;

        case 0x02:        // Uncompressed true-color image
            LoadUncompressedTrueColorImage(targa, file);
            break;

        default:
            return { };
    }

    file.close();

    if (targa.pixelLayout == PIXEL_LAYOUT::nUNDEFINED)
        return { };

    RawImage image;

    image.format = GetPixelFormat(targa.pixelLayout);
    image.view_type = graphics::IMAGE_VIEW_TYPE::TYPE_2D;

    image.width = targa.width;
    image.height = targa.height;

    image.mipLevels = static_cast<std::uint32_t>(std::floor(std::log2(std::max(image.width, image.height))) + 1);

    image.data = std::move(targa.data);

    return image;
}