#include <string>
using namespace std::string_literals;

#include <string_view>
using namespace std::string_view_literals;

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "material.hxx"
#include "shader_program.hxx"
#include "loaders/material_loader.hxx"


namespace
{
VkCullModeFlags constexpr ConvertToGAPI(CULL_MODE cullMode) noexcept
{
    switch (cullMode) {
        case CULL_MODE::NONE:
            return VkCullModeFlagBits::VK_CULL_MODE_NONE;

        case CULL_MODE::FRONT:
            return VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT;

        case CULL_MODE::BACK:
            return VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;

        case CULL_MODE::FRONT_AND_BACK:
            return VkCullModeFlagBits::VK_CULL_MODE_FRONT_AND_BACK;

        default:
            return VkCullModeFlagBits::VK_CULL_MODE_NONE;
    }
}

VkPolygonMode constexpr ConvertToGAPI(POLYGON_MODE polygonMode) noexcept
{
    switch (polygonMode) {
        case POLYGON_MODE::FILL:
            return VkPolygonMode::VK_POLYGON_MODE_FILL;

        case POLYGON_MODE::LINE:
            return VkPolygonMode::VK_POLYGON_MODE_LINE;

        case POLYGON_MODE::POINT:
            return VkPolygonMode::VK_POLYGON_MODE_POINT;

        default:
            return VkPolygonMode::VK_POLYGON_MODE_FILL;
    }
}

VkFrontFace constexpr ConvertToGAPI(POLYGON_FRONT_FACE frontFace) noexcept
{
    switch (frontFace) {
        case POLYGON_FRONT_FACE::COUNTER_CLOCKWISE:
            return VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;

        case POLYGON_FRONT_FACE::CLOCKWISE:
            return VkFrontFace::VK_FRONT_FACE_CLOCKWISE;

        default:
            return VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
}

VkCompareOp constexpr ConvertToGAPI(COMPARE_OPERATION compareOperation) noexcept
{
    switch (compareOperation) {
        case COMPARE_OPERATION::NEVER:
            return VkCompareOp::VK_COMPARE_OP_NEVER;

        case COMPARE_OPERATION::LESS:
            return VkCompareOp::VK_COMPARE_OP_LESS;

        case COMPARE_OPERATION::EQUAL:
            return VkCompareOp::VK_COMPARE_OP_EQUAL;

        case COMPARE_OPERATION::LESS_OR_EQUAL:
            return VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;

        case COMPARE_OPERATION::GREATER:
            return VkCompareOp::VK_COMPARE_OP_GREATER;

        case COMPARE_OPERATION::NOT_EQUAL:
            return VkCompareOp::VK_COMPARE_OP_NOT_EQUAL;

        case COMPARE_OPERATION::GREATER_OR_EQUAL:
            return VkCompareOp::VK_COMPARE_OP_GREATER_OR_EQUAL;

        case COMPARE_OPERATION::ALWAYS:
            return VkCompareOp::VK_COMPARE_OP_ALWAYS;

        default:
            return VkCompareOp::VK_COMPARE_OP_NEVER;
    }
}

VkBlendFactor constexpr ConvertToGAPI(BLEND_FACTOR blendFactor) noexcept
{
    switch (blendFactor) {
        case BLEND_FACTOR::ZERO:
            return VkBlendFactor::VK_BLEND_FACTOR_ZERO;

        case BLEND_FACTOR::ONE:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE;

        case BLEND_FACTOR::SRC_COLOR:
            return VkBlendFactor::VK_BLEND_FACTOR_SRC_COLOR;

        case BLEND_FACTOR::ONE_MINUS_SRC_COLOR:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

        case BLEND_FACTOR::DST_COLOR:
            return VkBlendFactor::VK_BLEND_FACTOR_DST_COLOR;

        case BLEND_FACTOR::ONE_MINUS_DST_COLOR:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;

        case BLEND_FACTOR::SRC_ALPHA:
            return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;

        case BLEND_FACTOR::ONE_MINUS_SRC_ALPHA:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        case BLEND_FACTOR::DST_ALPHA:
            return VkBlendFactor::VK_BLEND_FACTOR_DST_ALPHA;

        case BLEND_FACTOR::ONE_MINUS_DST_ALPHA:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

        case BLEND_FACTOR::CONSTANT_COLOR:
            return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_COLOR;

        case BLEND_FACTOR::ONE_MINUS_CONSTANT_COLOR:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;

        case BLEND_FACTOR::CONSTANT_ALPHA:
            return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_ALPHA;

        case BLEND_FACTOR::ONE_MINUS_CONSTANT_ALPHA:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;

        case BLEND_FACTOR::SRC_ALPHA_SATURATE:
            return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;

        case BLEND_FACTOR::SRC1_COLOR:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;

        case BLEND_FACTOR::ONE_MINUS_SRC1_COLOR:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;

        case BLEND_FACTOR::SRC1_ALPHA:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;

        case BLEND_FACTOR::ONE_MINUS_SRC1_ALPHA:
            return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;

        default:
            return VkBlendFactor::VK_BLEND_FACTOR_ZERO;

    }
}

VkBlendOp constexpr ConvertToGAPI(BLEND_OPERATION blendOperation) noexcept
{
    switch (blendOperation) {
        case BLEND_OPERATION::ADD:
            return VkBlendOp::VK_BLEND_OP_ADD;

        case BLEND_OPERATION::SUBTRACT:
            return VkBlendOp::VK_BLEND_OP_SUBTRACT;

        default:
            return VkBlendOp::VK_BLEND_OP_ADD;
    }
}

VkColorComponentFlags constexpr ConvertToGAPI(COLOR_COMPONENT colorComponent) noexcept
{
    switch (colorComponent) {
        case COLOR_COMPONENT::R:
            return VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT;

        case COLOR_COMPONENT::G:
            return VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT;

        case COLOR_COMPONENT::B:
            return VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT;

        case COLOR_COMPONENT::A:
            return VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;

        case COLOR_COMPONENT::RGB:
            return
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT |
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT |
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT;

        case COLOR_COMPONENT::RGBA:
            return
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT |
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT |
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT |
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;

        default:
            return
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT |
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT |
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT |
                VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
    }
}
}

namespace
{
class TexCoordsDebugMaterial final : public Material {
public:

    [[nodiscard]] std::vector<ShaderStage> const &shaderStages() const override
    {
        thread_local static std::vector<ShaderStage> shaderStages{
            ShaderStage{shader::STAGE::VERTEX, R"(test/shader.vert)"s, "main"s, std::vector<std::int32_t>{1}},
            ShaderStage{shader::STAGE::FRAGMENT, R"(test/shader.frag)"s, "main"s}

        };

        return shaderStages;
    }
};

class NormalsDebugMaterial final : public Material {
public:

    [[nodiscard]] std::vector<ShaderStage> const &shaderStages() const override
    {
        thread_local static std::vector<ShaderStage> shaderStages{
            ShaderStage{shader::STAGE::VERTEX, R"(test/shader.vert)"s, "main"s, std::vector<std::int32_t>{0}},
            ShaderStage{shader::STAGE::FRAGMENT, R"(test/shader.frag)"s, "main"s}
        };

        return shaderStages;
    }
};

class ColorsDebugMaterial final : public Material {
public:

    [[nodiscard]] std::vector<ShaderStage> const &shaderStages() const override
    {
        thread_local static std::vector<ShaderStage> shaderStages{
            ShaderStage{shader::STAGE::VERTEX, R"(test/shader.vert)"s, "main"s, std::vector<std::int32_t>{2}},
            ShaderStage{shader::STAGE::FRAGMENT, R"(test/shader.frag)"s, "main"s}
        };

        return shaderStages;
    }
};

class TestMaterial final : public Material {
public:

    [[nodiscard]] std::vector<ShaderStage> const &shaderStages() const override
    {
        thread_local static std::vector<ShaderStage> shaderStages{
            ShaderStage{shader::STAGE::VERTEX, R"(shader.vert)"s, "main"s},
            ShaderStage{shader::STAGE::FRAGMENT, R"(shader.frag)"s, "main"s}
        };

        return shaderStages;
    }
};
}


std::shared_ptr<MaterialProperties> const MaterialFactory::properties(std::shared_ptr<Material> const material)// const noexcept
{
    if (materialProperties_.count(material) == 0)
        return { };

    auto &&properties = materialProperties_.at(material);

    return { material, &properties };
}

std::vector<VkPipelineShaderStageCreateInfo> const &MaterialFactory::pipelineShaderStages(std::shared_ptr<Material> const material)
{
    if (pipelineShaderStages_.count(material) == 0) {
        std::vector<VkPipelineShaderStageCreateInfo> pipelineStages;

        auto &&stages = material->shaderStages();

        std::transform(std::cbegin(stages), std::cend(stages), std::back_inserter(pipelineStages), [this] (auto &&stage)
        {
            return shaderManager_.shaderStageProgram(stage);
        });

        pipelineShaderStages_.emplace(material, pipelineStages);
    }

    return pipelineShaderStages_.at(material);
}

void MaterialFactory::InitMaterialProperties(std::shared_ptr<Material> material) noexcept
{
    auto &&properties = materialProperties_[material];

    properties.rasterizationState = VkPipelineRasterizationStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr, 0,
        VK_TRUE,
        VK_FALSE,
        ConvertToGAPI(material->rasterizationState.polygonMode),
        ConvertToGAPI(material->rasterizationState.cullMode),
        ConvertToGAPI(material->rasterizationState.frontFace),
        VK_FALSE,
        0.f, 0.f, 0.f,
        material->rasterizationState.lineWidth
    };

    properties.depthStencilState = VkPipelineDepthStencilStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr, 0,
        VkBool32(material->depthStencilState.depthTestEnable),
        VkBool32(material->depthStencilState.depthWriteEnable),
        ConvertToGAPI(material->depthStencilState.depthCompareOperation),//kREVERSED_DEPTH ? VK_COMPARE_OP_GREATER : VK_COMPARE_OP_LESS,
        VK_FALSE,
        VkBool32(material->depthStencilState.stencilTestEnable),
        VkStencilOpState{}, VkStencilOpState{},
        0, 1
    };

    for (auto &&attachment : material->colorBlendState.attachments) {
        properties.colorBlendAttachmentStates.push_back(VkPipelineColorBlendAttachmentState{
            VkBool32(attachment.blendEnable),
            ConvertToGAPI(attachment.srcColorBlendFactor),
            ConvertToGAPI(attachment.dstColorBlendFactor),

            ConvertToGAPI(attachment.colorBlendOperation),

            ConvertToGAPI(attachment.srcAlphaBlendFactor),
            ConvertToGAPI(attachment.dstAlphaBlendFactor),

            ConvertToGAPI(attachment.alphaBlendOperation),

            ConvertToGAPI(attachment.colorWriteMask)
        });
    }

    properties.colorBlendState = VkPipelineColorBlendStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr, 0,
        VK_FALSE,
        VK_LOGIC_OP_COPY,
        static_cast<std::uint32_t>(std::size(properties.colorBlendAttachmentStates)),
        std::data(properties.colorBlendAttachmentStates),
        {}
    };

    std::copy(std::cbegin(material->colorBlendState.blendConstants), std::cend(material->colorBlendState.blendConstants),
              std::begin(properties.colorBlendState.blendConstants));
}

std::shared_ptr<Material> MaterialFactory::CreateMaterial(std::string_view type)
{
    if (materials_.count(std::string{type}) != 0)
        return materials_.at(std::string{type});

    std::shared_ptr<Material> material;

    if (type == "TexCoordsDebugMaterial"s)
        material = std::make_shared<TexCoordsDebugMaterial>();

    else if (type == "ColorsDebugMaterial"s)
        material = std::make_shared<ColorsDebugMaterial>();

    else if (type == "NormalsDebugMaterial"s)
        material = std::make_shared<NormalsDebugMaterial>();

    else if (type == "TestMaterial"s)
        material = std::make_shared<TestMaterial>();

    material->colorBlendState.attachments.push_back(ColorBlendAttachmentState{
        false,
        BLEND_FACTOR::ONE,
        BLEND_FACTOR::ZERO,
        BLEND_OPERATION::ADD,
        BLEND_FACTOR::ONE,
        BLEND_FACTOR::ZERO,
        BLEND_OPERATION::ADD,
        COLOR_COMPONENT::RGBA
    });

    InitMaterialProperties(material);

    shaderManager_.CreateShaderPrograms(material.get());

    materials_.emplace(std::string{type}, material);

    return material;
}

std::shared_ptr<Material2> MaterialFactory::material_by_techique(std::string_view name, std::uint32_t technique_index)
{
#if 0
    auto const key = std::pair{std::string{name}, technique_index};
    
    if (materials_by_techinques_.count(key) != 0)
        return materials_by_techinques_.at(key);

    auto const description = material_description(name);

    auto &&techniques = description->techniques;
    auto &&technique = techniques.at(technique_index);

    auto &&shader_modules = description->shader_modules;
    auto &&shaders_bundle = technique.shaders_bundle;

    std::vector<program::shader_stage> shader_stages;

    std::transform(std::cbegin(shaders_bundle), std::cend(shaders_bundle),
                   std::back_inserter(shader_stages), [&shader_modules] (auto shader_bundle)
    {
        auto [shader_module_index, shader_technique_index] = shader_bundle;

        auto &&[shader_semantic, shader_name] = shader_modules.at(shader_module_index);

        return program::shader_stage{shader_semantic, shader_name};
    });

    auto material = std::make_shared<Material2>();

    material->shader_stages = std::move(shader_stages);

    materials_by_techinques_.emplace(key, material);

    return material;
#endif

    return { };
}

std::shared_ptr<loader::material_description> MaterialFactory::material_description(std::string_view name)
{
#if 0
    auto _name = std::string{name};

    if (material_descriptions_.count(_name) != 0)
        return material_descriptions_.at(_name);

    auto description = loader::load_material_description(name);

    material_descriptions_.emplace(_name, description);
#endif

    return { };
}

void MaterialFactory::InitMaterialProperties(std::shared_ptr<Material2> material)
{
    auto &&properties = materialProperties2_[material];

    properties.rasterizationState = VkPipelineRasterizationStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr, 0,
        VK_TRUE,
        VK_FALSE,
        ConvertToGAPI(material->rasterization_state.polygonMode),
        ConvertToGAPI(material->rasterization_state.cullMode),
        ConvertToGAPI(material->rasterization_state.frontFace),
        VK_FALSE,
        0.f, 0.f, 0.f,
        material->rasterization_state.lineWidth
    };

    properties.depthStencilState = VkPipelineDepthStencilStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr, 0,
        VkBool32(material->depth_stencil_state.depthTestEnable),
        VkBool32(material->depth_stencil_state.depthWriteEnable),
        ConvertToGAPI(material->depth_stencil_state.depthCompareOperation),//kREVERSED_DEPTH ? VK_COMPARE_OP_GREATER : VK_COMPARE_OP_LESS,
        VK_FALSE,
        VkBool32(material->depth_stencil_state.stencilTestEnable),
        VkStencilOpState{}, VkStencilOpState{},
        0, 1
    };

    for (auto &&attachment : material->colorBlendState.attachments) {
        properties.colorBlendAttachmentStates.push_back(VkPipelineColorBlendAttachmentState{
            VkBool32(attachment.blendEnable),
            ConvertToGAPI(attachment.srcColorBlendFactor),
            ConvertToGAPI(attachment.dstColorBlendFactor),

            ConvertToGAPI(attachment.colorBlendOperation),

            ConvertToGAPI(attachment.srcAlphaBlendFactor),
            ConvertToGAPI(attachment.dstAlphaBlendFactor),

            ConvertToGAPI(attachment.alphaBlendOperation),

            ConvertToGAPI(attachment.colorWriteMask)
        });
    }

    properties.colorBlendState = VkPipelineColorBlendStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr, 0,
        VK_FALSE,
        VK_LOGIC_OP_COPY,
        static_cast<std::uint32_t>(std::size(properties.colorBlendAttachmentStates)),
        std::data(properties.colorBlendAttachmentStates),
        {}
    };

    std::copy(std::cbegin(material->colorBlendState.blendConstants), std::cend(material->colorBlendState.blendConstants),
              std::begin(properties.colorBlendState.blendConstants));
}


namespace graphics
{
    std::shared_ptr<graphics::material> material_factory::material(std::string_view name, std::uint32_t technique_index)
    {
        auto const key = std::pair{std::string{name}, technique_index};

        if (materials_.count(key) != 0)
            return materials_.at(key);

        auto &&description = material_description(name);

        auto &&techniques = description.techniques;
        auto &&technique = techniques.at(technique_index);

        auto &&shader_modules = description.shader_modules;
        auto &&shaders_bundle = technique.shaders_bundle;

        std::vector<graphics::shader_stage> shader_stages;

        std::transform(std::cbegin(shaders_bundle), std::cend(shaders_bundle),
                       std::back_inserter(shader_stages), [&shader_modules] (auto shader_bundle)
        {
            auto [shader_module_index, shader_technique_index] = shader_bundle;

            auto &&[shader_semantic, shader_name] = shader_modules.at(shader_module_index);

            return graphics::shader_stage{shader_name, static_cast<std::uint32_t>(shader_technique_index), shader_semantic};
        });

        return std::shared_ptr<graphics::material>{};
    }

    loader::material_description const &material_factory::material_description(std::string_view name)
    {
        auto key = std::string{name};

        if (material_descriptions_.count(key) == 0)
            material_descriptions_.emplace(key, loader::load_material_description(name));

        return material_descriptions_.at(key);
    }
}
