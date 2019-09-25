#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
#include "device/device.hxx"
#include "resources/resource.hxx"

#include "graphics.hxx"
#include "attachments.hxx"
#include "loaders/material_loader.hxx"
#include "resources/program.hxx"
#include "shader_program.hxx"


struct MaterialProperties final {
    VkPipelineRasterizationStateCreateInfo rasterizationState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
};

class Material {
public:

    virtual ~Material() = default;

    [[nodiscard]] virtual std::vector<ShaderStage> const &shaderStages() const = 0;

    graphics::rasterization_state rasterizationState;
    graphics::depth_stencil_state depthStencilState;

    graphics::color_blend_state colorBlendState;


private:

    // resources
    //// shader handle
    //// rasterization state
    //// depth and stencil state
    //// blending
    // pipeline layout (descriptor set layout)
};

class MaterialFactory final {
public:

    MaterialFactory(ShaderManager &shaderManager) noexcept : shaderManager_{shaderManager} { }

    [[nodiscard]] std::shared_ptr<Material> CreateMaterial(std::string_view type);

    [[nodiscard]] std::shared_ptr<MaterialProperties> const properties(std::shared_ptr<Material> const material);// const noexcept;

    [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> const &pipelineShaderStages(std::shared_ptr<Material> const material);

    std::map<std::string, std::shared_ptr<Material>> const materials() const noexcept { return materials_; }


private:

    ShaderManager &shaderManager_;

    std::map<std::string, std::shared_ptr<Material>> materials_;
    std::map<std::shared_ptr<Material>, MaterialProperties, std::owner_less<std::shared_ptr<Material>>> materialProperties_;

    std::map<std::shared_ptr<Material>, std::vector<VkPipelineShaderStageCreateInfo>, std::owner_less<std::shared_ptr<Material>>> pipelineShaderStages_;

    void InitMaterialProperties(std::shared_ptr<Material> material) noexcept;


    std::map<std::string, std::shared_ptr<loader::material_description>> material_descriptions_;

    [[nodiscard]] std::shared_ptr<loader::material_description> material_description(std::string_view name);
};


namespace graphics
{
    struct material final {

        material(std::vector<graphics::shader_stage> shader_stages) : shader_stages{shader_stages} { }

        std::vector<graphics::shader_stage> shader_stages;

        // TODO:: descriptor set and pipeline layout.
    };
}

namespace graphics
{
    class material_factory final {
    public:

        [[nodiscard]] std::shared_ptr<graphics::material> material(std::string_view name, std::uint32_t technique_index);

    private:

        std::map<std::pair<std::string, std::uint32_t>, std::shared_ptr<graphics::material>> materials_;

        // TODO:: move to general loader manager.
        std::unordered_map<std::string, loader::material_description> material_descriptions_;

        [[nodiscard]] loader::material_description const &material_description(std::string_view name);
    };
}

namespace graphics
{
    template<>
    struct hash<graphics::material> {
        std::size_t operator() (graphics::material const &material) const
        {
            std::size_t seed = 0;

            graphics::hash<graphics::shader_stage> constexpr shader_stage_hasher;

            for (auto &&shader_stage : material.shader_stages)
                boost::hash_combine(seed, shader_stage_hasher(shader_stage));

            return seed;
        }
    };
}
