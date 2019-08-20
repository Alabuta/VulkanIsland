#include <fmt/format.h>

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "loaders/SPIRV_loader.hxx"
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


void ShaderManager::CreateShaderPrograms(Material const *const material)
{
    for (auto &&shaderStage : material->shaderStages()) {
        if (shaderStagePrograms_.count(shaderStage) == 0) {
            if (shaderModules_.count(shaderStage.moduleName) == 0) {
                auto const shaderByteCode = loader::load_SPIRV(shaderStage.moduleName);

                if (shaderByteCode.empty())
                    throw std::runtime_error("failed to open vertex shader file"s);

                auto shaderModule = create_shader_module(shaderByteCode);

                shaderModules_.emplace(shaderStage.moduleName, shaderModule);
            }

            auto &&shaderModule = shaderModules_.at(shaderStage.moduleName);

            if (!shaderStage.constants.empty()) {
                auto &&constants = shaderStage.constants;

                if (specializationInfos_.count(shaderStage) == 0) {
                    if (specializationMapEntries_.count(shaderStage) == 0) {
                        std::vector<VkSpecializationMapEntry> specializationMapEntry;

                        std::uint32_t constantID = 0;
                        std::uint32_t offset = 0;

                        for (auto constant : constants) {
                            specializationMapEntry.push_back({
                                constantID++, offset, sizeof(constant)
                            });

                            offset += static_cast<std::uint32_t>(sizeof(constant));
                        }

                        specializationMapEntries_.emplace(shaderStage, std::move(specializationMapEntry));
                    }

                    auto &&specializationMapEntry = specializationMapEntries_.at(shaderStage);

                    specializationInfos_.emplace(shaderStage, VkSpecializationInfo{
                        static_cast<std::uint32_t>(std::size(specializationMapEntry)),
                        std::data(specializationMapEntry),
                        std::size(constants) * sizeof(std::decay_t<decltype(constants)>::value_type),
                        std::data(constants)
                    });
                }

                auto &&specializationInfo = specializationInfos_.at(shaderStage);
                
                shaderStagePrograms_.emplace(shaderStage, VkPipelineShaderStageCreateInfo{
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    nullptr, 0,
                    ConvertToGAPI(shaderStage.semantic),
                    shaderModule->handle(),
                    shaderStage.entryPoint.c_str(),
                    &specializationInfo
                });
            }

            else {
                shaderStagePrograms_.emplace(shaderStage, VkPipelineShaderStageCreateInfo{
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
}

VkPipelineShaderStageCreateInfo const &ShaderManager::shaderStageProgram(ShaderStage const &shaderStage) const
{
    return shaderStagePrograms_.at(shaderStage);
}

VkPipelineShaderStageCreateInfo ShaderManager::pipeline_shader_stage(program::shader_stage const &shader_stage)
{
    if (pipeline_shader_stages_.count(shader_stage) != 0)
        return pipeline_shader_stages_.at(shader_stage);

    VkPipelineShaderStageCreateInfo pipeline_stage;

    pipeline_shader_stages_.emplace(shader_stage, pipeline_stage);

    return pipeline_stage;
}

std::shared_ptr<VulkanShaderModule> ShaderManager::shader_module(std::string_view name, std::uint32_t technique_index)
{
    auto const key = std::pair{std::string{name}, technique_index};

    if (modules_by_techinques_.count(key) != 0)
        return modules_by_techinques_.at(key);

    auto full_name = fmt::format("{}.{}"s, name, technique_index);

    boost::uuids::name_generator_sha1 gen(boost::uuids::ns::dns());
    auto hashed_name = boost::uuids::to_string(gen(full_name));

    auto const shader_byte_code = loader::load_SPIRV(hashed_name);

    if (shader_byte_code.empty())
        throw std::runtime_error("failed to open vertex shader file"s);

    auto shader_module = create_shader_module(shader_byte_code);

    modules_by_techinques_.emplace(key, shader_module);

    return shader_module;
}

std::shared_ptr<VulkanShaderModule> ShaderManager::create_shader_module(std::vector<std::byte> const &shader_byte_code)
{
    std::shared_ptr<VulkanShaderModule> shader_module;

    if (std::size(shader_byte_code) % sizeof(std::uint32_t) != 0)
        std::cerr << "invalid byte code buffer size\n"s;

    else {
        VkShaderModuleCreateInfo const create_info{
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            nullptr, 0,
            std::size(shader_byte_code),
            reinterpret_cast<std::uint32_t const *>(std::data(shader_byte_code))
        };

        VkShaderModule handle;

        if (auto result = vkCreateShaderModule(vulkan_device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
            std::cerr << fmt::format("failed to create shader module: {0:#x}\n"s, result);

        else shader_module.reset(
            new VulkanShaderModule{handle},
            [this] (VulkanShaderModule *ptr_module)
            {
                vkDestroyShaderModule(vulkan_device_.handle(), ptr_module->handle(), nullptr);

                delete ptr_module;
            }
        );
    }

    return shader_module;
}
