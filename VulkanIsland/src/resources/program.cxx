#include "program.hxx"
#include "renderer/material.hxx"



namespace
{
VkShaderStageFlagBits constexpr ConvertToGAPI(shader::STAGE stage) noexcept
{
    switch (stage) {
        case shader::STAGE::VERTEX:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

        case shader::STAGE::TESS_CONTROL:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

        case shader::STAGE::TESS_EVAL:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

        case shader::STAGE::GEOMETRY:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;

        case shader::STAGE::FRAGMENT:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

        case shader::STAGE::COMPUTE:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

        default:
            return VkShaderStageFlagBits::VK_SHADER_STAGE_ALL;
    }
}
}

namespace
{
std::vector<std::byte> ReadShaderFile(std::string_view name)
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
}



void ShaderManager::CreateShaderPrograms(Material const *const material)
{
    for (auto &&shaderStage : material->shaderStages()) {
        if (pipelineShaderStages_.count(shaderStage) == 0) {
            if (shaderModules_.count(shaderStage.moduleName) == 0) {
                auto const shaderByteCode = ReadShaderFile(shaderStage.moduleName);

                if (shaderByteCode.empty())
                    throw std::runtime_error("failed to open vertex shader file"s);

                auto shaderModule = CreateShaderModule(shaderByteCode);

                shaderModules_.emplace(shaderStage.moduleName, shaderModule);
            }

            auto &&shaderModule = shaderModules_.at(shaderStage.moduleName);

            pipelineShaderStages_.emplace(shaderStage, VkPipelineShaderStageCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr, 0,
                ConvertToGAPI(shaderStage.semantic),
                shaderModule->handle(),
                shaderStage.entryPoint.c_str(),
                nullptr
            });
        }
    }
}

VkPipelineShaderStageCreateInfo const &ShaderManager::GetPipelineShaderStage(ShaderStage const &shaderStage) const
{
    return pipelineShaderStages_.at(shaderStage);
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
