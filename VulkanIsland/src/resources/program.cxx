#include "program.hxx"



std::vector<std::byte> ShaderManager::ReadShaderFile(std::string_view name) const noexcept
{
    fs::path contents{"shaders"sv};

    if (!fs::exists(fs::current_path() / contents))
        contents = fs::current_path() / "../"sv / contents;

    auto path = contents / name;

    std::ifstream file{path.native(), std::ios::in | std::ios::binary};

    if (file.bad() || file.fail())
        return { };

    auto const start_pos = file.tellg();
    file.ignore(std::numeric_limits<std::streamsize>::max());

    std::vector<std::byte> shaderByteCode(static_cast<std::size_t>(file.gcount()));

    file.seekg(start_pos);

    if (!shaderByteCode.empty())
        file.read(reinterpret_cast<char *>(std::data(shaderByteCode)), static_cast<std::streamsize>(std::size(shaderByteCode)));

    return shaderByteCode;
}

std::shared_ptr<VulkanShaderModule> ShaderManager::CreateShaderModule(std::vector<std::byte> const &shaderByteCode)
{
    std::shared_ptr<VulkanShaderModule> shaderModule;

    if (std::size(shaderByteCode) % sizeof(std::uint32_t) != 0)
        std::cerr << "invalid byte code buffer size\n"s;

    else {
        VkShaderModuleCreateInfo const createInfo{
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            nullptr, 0,
            std::size(shaderByteCode),
            reinterpret_cast<std::uint32_t const *>(std::data(shaderByteCode))
        };

        VkShaderModule handle;

        if (auto result = vkCreateShaderModule(vulkanDevice_.handle(), &createInfo, nullptr, &handle); result != VK_SUCCESS)
            std::cerr << "failed to create shader module: "s << result << '\n';

        else shaderModule.reset(
            new VulkanShaderModule{handle},
            [this] (VulkanShaderModule *ptr_module)
            {
                vkDestroyShaderModule(vulkanDevice_.handle(), ptr_module->handle(), nullptr);

                delete ptr_module;
            }
        );
    }

    return shaderModule;
}

std::vector<VkPipelineShaderStageCreateInfo>
ShaderManager::GetShaderStages(std::vector<ShaderStageSource> const &shaderStageSources)
{
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    auto paths = std::accumulate(std::cbegin(shaderStageSources), std::cend(shaderStageSources), ""s, [] (auto &&lhs, auto &&rhs)
    {
        return lhs + rhs.path;
    });

    return stages;
}
