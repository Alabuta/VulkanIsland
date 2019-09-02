#pragma once

#include <memory>
#include <unordered_map>

#include "main.hxx"
#include "device/device.hxx"
#include "resources/resource.hxx"

#include "graphics.hxx"
#include "graphics_pipeline.hxx"
#include "pipeline_states.hxx"
#include "attachments.hxx"
#include "loaders/material_loader.hxx"
#include "resources/program.hxx"


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


struct material_properties final {
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
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
    class material final {
    public:

        material() = default;

        //material(graphics::pipeline const &pipeline) : pipeline_{pipeline} { }

    private:

        //graphics::pipeline const &pipeline_;

        std::vector<graphics::shader_stage> shader_stages_;
    };
}

namespace graphics
{
    class material_factory final {
    public:

        [[nodiscard]] std::shared_ptr<graphics::material> material(std::string_view name, std::uint32_t technique_index);

    private:

        // TODO:: move to general loader manager.
        std::map<std::string, loader::material_description> material_descriptions_;

        std::map<std::pair<std::string, std::uint32_t>, std::shared_ptr<graphics::material>> materials_;

        [[nodiscard]] loader::material_description const &material_description(std::string_view name);
    };
}
