#pragma once

#include "main.hxx"

[[nodiscard]] std::vector<std::byte> ReadShaderFile(std::string_view _name)
{
    auto current_path = fs::current_path();

    fs::path directory{"shaders"s};
    fs::path name{std::data(_name)};

    if (!fs::exists(current_path / directory))
        directory = current_path / fs::path{"../../VulkanIsland"s} / directory;

    std::ifstream file((directory / name).native(), std::ios::binary);

    if (!file.is_open())
        return { };

    auto const start_pos = file.tellg();
    file.ignore(std::numeric_limits<std::streamsize>::max());

    std::vector<std::byte> shaderByteCode(file.gcount());

    file.seekg(start_pos);

    if (!shaderByteCode.empty())
        file.read(reinterpret_cast<char *>(std::data(shaderByteCode)), shaderByteCode.size());

    return shaderByteCode;
}

template<class T, typename std::enable_if_t<is_container_v<std::decay_t<T>>>...>
[[nodiscard]] VkShaderModule CreateShaderModule(VkDevice device, T &&shaderByteCode)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, std::byte>, "iterable object does not contain std::byte elements");

    if (shaderByteCode.size() % sizeof(std::uint32_t) != 0)
        throw std::runtime_error("invalid byte code buffer size");

    VkShaderModuleCreateInfo const createInfo{
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr, 0,
        shaderByteCode.size(),
        reinterpret_cast<std::uint32_t const *>(std::data(shaderByteCode))
    };

    VkShaderModule shaderModule;

    if (auto result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule); result != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module: "s + std::to_string(result));

    return shaderModule;
}
