#pragma once

#include "main.hxx"

class VulkanShaderModule final {
public:

    VulkanShaderModule(VkShaderModule handle) noexcept : handle_{handle} { }

    VkShaderModule handle() const noexcept { return handle_; }

private:

    VkShaderModule handle_;

    VulkanShaderModule() = delete;
    VulkanShaderModule(VulkanShaderModule const &) = delete;
    VulkanShaderModule(VulkanShaderModule &&) = delete;
};

[[nodiscard]] std::vector<std::byte> ReadShaderFile(std::string_view name);


template<class T, typename std::enable_if_t<is_container_v<std::decay_t<T>>>...>
[[nodiscard]] VkShaderModule CreateShaderModule(VkDevice device, T &&shaderByteCode)
{
    static_assert(std::is_same_v<typename std::decay_t<T>::value_type, std::byte>, "iterable object does not contain std::byte elements");

    if (shaderByteCode.size() % sizeof(std::uint32_t) != 0)
        throw std::runtime_error("invalid byte code buffer size"s);

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
