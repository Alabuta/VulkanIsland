
#include <istream>

#include "main.h"
#include "TARGA_loader.h"

bool LoadUncompressedTARGA(Image &_image, std::ifstream &_file, std::vector<std::byte> &pixels)
{
    size_t const count = _image.bpp * _image.width * _image.height;

    switch (_image.bpp) {
        /*case 1:
        _texture.bpp = GL_RED;
        break;

        case 2:
        _texture.bpp = GL_RG;
        break;

        case 3:
        _image.bpp_ = GL_RGB;
        _image.format_ = GL_BGR;
        break;*/

        case 4:
            //_image.bpp = GL_RGBA8;
            //_image.format = GL_BGRA;
            break;

        default:
            _file.close();
            return false;
    }

#if 0
    _image.data.resize(count);
    _file.read(reinterpret_cast<char *>(std::data(_image.data)), count);
    //_image.data_.assign(std::istreambuf_iterator<char>(_file), std::istreambuf_iterator<char>());
    _image.data.shrink_to_fit();
#endif
    pixels.resize(count);
    _file.read(reinterpret_cast<char *>(std::data(pixels)), count);
    pixels.shrink_to_fit();

    _file.close();
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

    std::cout << (directory / name).native() << '\n';

    if (!file.is_open())
        return {};

#if 0
    struct header_t {
        std::byte ID_length;
        std::byte color_map_type;
        std::byte image_type;
        std::array<std::byte, 5> color_map_spec;
        std::array<std::byte, 10> image_spec;
    } header;

    file.read(reinterpret_cast<char *>(&header), sizeof(header_t));

    struct image_and_color_map_data_t {
        std::vector<std::byte> image_id;
        std::vector<std::byte> color_map_data;
        std::vector<std::byte> image_data;
    } image_and_color_map_data;

    file.read(reinterpret_cast<char *>(&image_and_color_map_data), sizeof(image_and_color_map_data_t));

    image_and_color_map_data.image_id.resize(static_cast<std::size_t>(header.ID_length));
    file.read(reinterpret_cast<char *>(std::data(image_and_color_map_data.image_id)), sizeof(std::byte) * static_cast<std::size_t>(header.ID_length));
#endif
    /* std::byte width, height;
    std::uint8_t bpp;*/

    file.seekg(sizeof(std::byte) * 2, std::ios::beg);

    char headerTARGA = 0;
    file.read(&headerTARGA, sizeof(headerTARGA));

    file.seekg(sizeof(std::byte) * 9, std::ios::cur);

    char header[6]{0};

    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    Image image;

    /*image.width = header.width;
    image.height = header.height;
    image.bpp = header.bpp / 8;*/

    image.width = (header[1] << 8) + header[0];
    image.height = (header[3] << 8) + header[2];
    image.bpp = header[4] / 8;

    if (image.width * image.height * image.bpp < 0)
        return {};

    switch (headerTARGA) {
        case 2:
            LoadUncompressedTARGA(image, file, pixels);
            break;

        case 10:
            //LoadCompressedTARGA(image, file);
            break;

        default:
            break;
    }

    // image.type = GL_UNSIGNED_BYTE;

    file.close();

    return std::make_pair(image.width, image.height);
}