#include <exception>
#include <cstddef>
#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include "graphics_pipeline.hxx"


namespace
{
    auto const kENTRY_POINTS = std::array{
        "technique0"s,
        "technique1"s,
        "technique2"s,
        "technique3"s
    };
}

namespace convert_to
{
    [[nodiscard]] VkPipelineVertexInputStateCreateInfo
    vulkan(graphics::vertex_input_state const &vertex_input_state,
           std::vector<VkVertexInputBindingDescription> &binding_descriptions,
           std::vector<VkVertexInputAttributeDescription> &attribute_descriptions)
    {
        auto [binding_index, size_in_bytes, input_rate] = vertex_input_state.binding_description;

        binding_descriptions.push_back(VkVertexInputBindingDescription{
            binding_index, size_in_bytes, convert_to::vulkan(input_rate)
        });

        std::transform(std::cbegin(vertex_input_state.attribute_descriptions), std::cend(vertex_input_state.attribute_descriptions),
                       std::back_inserter(attribute_descriptions),
                       [binding_index] (auto &&attribute_description)
        {
            auto [
                location_index, binding_index, offset_in_bytes, format
            ] = attribute_description;

            return VkVertexInputAttributeDescription{
                location_index, binding_index, convert_to::vulkan(format), offset_in_bytes
            };
        });

        return VkPipelineVertexInputStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            nullptr, 0,
            static_cast<std::uint32_t>(std::size(binding_descriptions)), std::data(binding_descriptions),
            static_cast<std::uint32_t>(std::size(attribute_descriptions)), std::data(attribute_descriptions)
        };
    }

    [[nodiscard]] VkPipelineRasterizationStateCreateInfo vulkan(graphics::rasterization_state const &rasterization_state)
    {
        return VkPipelineRasterizationStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            nullptr, 0,
            VK_TRUE,
            VK_FALSE,
            convert_to::vulkan(rasterization_state.polygon_mode),
            convert_to::vulkan(rasterization_state.cull_mode),
            convert_to::vulkan(rasterization_state.front_face),
            VK_FALSE,
            0.f, 0.f, 0.f,
            rasterization_state.line_width
        };
    }

    [[nodiscard]] VkStencilOpState vulkan(graphics::stencil_state const &stencil_state)
    {
        return VkStencilOpState{
            convert_to::vulkan(stencil_state.fail),
            convert_to::vulkan(stencil_state.pass),
            convert_to::vulkan(stencil_state.depth_fail),

            convert_to::vulkan(stencil_state.compare_operation),

            stencil_state.compare_mask,
            stencil_state.write_mask,
            stencil_state.reference
        };
    }

    [[nodiscard]] VkPipelineDepthStencilStateCreateInfo vulkan(graphics::depth_stencil_state const &depth_stencil_state)
    {
        return VkPipelineDepthStencilStateCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            nullptr, 0,

            convert_to::vulkan(depth_stencil_state.depth_test_enable),
            convert_to::vulkan(depth_stencil_state.depth_write_enable),
            convert_to::vulkan(depth_stencil_state.depth_compare_operation), // kREVERSED_DEPTH ? VK_COMPARE_OP_GREATER : VK_COMPARE_OP_LESS,

            convert_to::vulkan(depth_stencil_state.depth_bounds_test_enable),

            convert_to::vulkan(depth_stencil_state.stencil_test_enable),
            convert_to::vulkan(depth_stencil_state.front_stencil_state),
            convert_to::vulkan(depth_stencil_state.back_stencil_state),

            depth_stencil_state.depth_bounds[0], depth_stencil_state.depth_bounds[1]
        };
    }

    [[nodiscard]] VkPipelineColorBlendStateCreateInfo
    vulkan(graphics::color_blend_state const &color_blend_state, std::vector<VkPipelineColorBlendAttachmentState> &attachment_states)
    {
        for (auto &&attachment_state : color_blend_state.attachment_states) {
            attachment_states.push_back(VkPipelineColorBlendAttachmentState{
                convert_to::vulkan(attachment_state.blend_enable),
                convert_to::vulkan(attachment_state.src_color_blend_factor),
                convert_to::vulkan(attachment_state.dst_color_blend_factor),

                convert_to::vulkan(attachment_state.color_blend_operation),

                convert_to::vulkan(attachment_state.src_alpha_blend_factor),
                convert_to::vulkan(attachment_state.dst_alpha_blend_factor),

                convert_to::vulkan(attachment_state.alpha_blend_operation),

                convert_to::vulkan(attachment_state.color_write_mask)
            });
        }

        VkPipelineColorBlendStateCreateInfo /*const */create_info{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            nullptr, 0,
            convert_to::vulkan(color_blend_state.logic_operation_enable),
            convert_to::vulkan(color_blend_state.logic_operation),
            static_cast<std::uint32_t>(std::size(attachment_states)),
            std::data(attachment_states),
            { }
        };

        std::copy(std::cbegin(color_blend_state.blend_constants), std::cend(color_blend_state.blend_constants),
                  std::begin(create_info.blendConstants));

        return create_info;
    }
}

namespace graphics
{
    std::shared_ptr<graphics::pipeline> pipeline_factory::create_pipeline(
        std::shared_ptr<graphics::material> material, graphics::pipeline_states const &pipeline_states,
        VkPipelineLayout layout, VkRenderPass render_pass, std::uint32_t subpass_index
    )
    {
        graphics::pipeline_invariant invariant{material, pipeline_states, layout, render_pass, subpass_index};

        if (pipelines_.count(invariant) != 0)
            return pipelines_.at(invariant);

        VkExtent2D const extent{600u, 400u};

        auto &&shader_stages = material->shader_stages;

        std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stages;

        std::vector<VkSpecializationMapEntry> specialization_map_entry;
        std::vector<std::byte> specialization_constants_data;
        VkSpecializationInfo specialization_info;

        std::transform(std::cbegin(shader_stages), std::cend(shader_stages), std::back_inserter(pipeline_shader_stages),
                       [&specialization_info, &specialization_constants_data, &specialization_map_entry, this] (auto &&shader_stage)
        {
            auto shader_module = shader_manager_.shader_module(shader_stage.module_name, shader_stage.techique_index);

            VkPipelineShaderStageCreateInfo create_info{
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    nullptr, 0,
                    convert_to::vulkan(shader_stage.semantic),
                    shader_module->handle(),
                    kENTRY_POINTS.at(shader_stage.techique_index).c_str(),
                    nullptr
            };

            if (shader_stage.constants.empty())
                return create_info;

            std::uint32_t offset = 0;

            for (auto [id, value] : shader_stage.constants) {
                std::visit([id, &offset, &specialization_info, &specialization_constants_data, &specialization_map_entry] (auto constant)
                {
                    specialization_map_entry.push_back({
                        id, offset, sizeof(constant)
                    });

                    auto dst_begin = std::next(std::begin(specialization_constants_data), offset);

                    std::uninitialized_copy_n(reinterpret_cast<std::byte *>(&constant), sizeof(constant), dst_begin);

                    offset += static_cast<std::uint32_t>(sizeof(constant));

                }, value);
            }

            specialization_info = VkSpecializationInfo{
                static_cast<std::uint32_t>(std::size(specialization_map_entry)), std::data(specialization_map_entry),
                std::size(specialization_constants_data), std::data(specialization_constants_data)
            };

            create_info.pSpecializationInfo = &specialization_info;

            return create_info;
        });

        std::vector<VkVertexInputBindingDescription> binding_descriptions;
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        auto vertex_input_state = convert_to::vulkan(pipeline_states.vertex_input_state, binding_descriptions, attribute_descriptions);

        VkPipelineInputAssemblyStateCreateInfo const input_assembly_state{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            nullptr, 0,
            convert_to::vulkan(pipeline_states.primitive_topology),
            VK_FALSE
        };

        // Render pass
    #if USE_DYNAMIC_PIPELINE_STATE
        auto const dynamic_states = std::array{
            VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
            VkDynamicState::VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo const dynamic_state{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            nullptr, 0,
            static_cast<std::uint32_t>(std::size(dynamic_states)),
            std::data(dynamic_states)
        };

        VkPipelineViewportStateCreateInfo const viewport_state{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            nullptr, 0,
            1, nullptr,
            1, nullptr
        };

    #else
        VkViewport const viewport{
            0, static_cast<float>(extent.height),
            static_cast<float>(extent.width), -static_cast<float>(extent.height),
            0, 1
        };

        VkRect2D const scissor{
            {0, 0}, extent
        };

        VkPipelineViewportStateCreateInfo const viewport_state{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            nullptr, 0,
            1, &viewport,
            1, &scissor
        };
    #endif

        auto rasterization_state = convert_to::vulkan(pipeline_states.rasterization_state);
        auto depth_stencil_state = convert_to::vulkan(pipeline_states.depth_stencil_state);

        std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment_states;
        auto color_blend_state = convert_to::vulkan(pipeline_states.color_blend_state, color_blend_attachment_states);

        VkPipelineMultisampleStateCreateInfo const multisample_state{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            nullptr, 0,
            convert_to::vulkan(vulkan_device_.samplesCount()),
            VK_FALSE, 1,
            nullptr,
            VK_FALSE,
            VK_FALSE
        };

        VkGraphicsPipelineCreateInfo const create_info{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            nullptr,
            VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
            static_cast<std::uint32_t>(std::size(pipeline_shader_stages)), std::data(pipeline_shader_stages),
            &vertex_input_state,
            &input_assembly_state,
            nullptr,
            &viewport_state,
            &rasterization_state,
            &multisample_state,
            &depth_stencil_state,
            &color_blend_state,
    #if USE_DYNAMIC_PIPELINE_STATE
            &dynamic_state,
    #else
            nullptr,
    #endif
            layout,
            render_pass,
            subpass_index,
            VK_NULL_HANDLE, -1
        };

        VkPipeline handle;

        if (auto result = vkCreateGraphicsPipelines(vulkan_device_.handle(), VK_NULL_HANDLE, 1, &create_info, nullptr, &handle); result != VK_SUCCESS)
            throw std::runtime_error(fmt::format("failed to create graphics pipeline: {0:#x}\n"s, result));

        auto pipeline = std::shared_ptr<graphics::pipeline>(
            new graphics::pipeline{handle}, [this] (graphics::pipeline *const ptr_pipeline)
            {
                vkDestroyPipeline(vulkan_device_.handle(), ptr_pipeline->handle(), nullptr);

                delete ptr_pipeline;
            }
        );

        pipelines_.emplace(invariant, pipeline);

        return pipeline;
    }
}

namespace graphics
{
    std::uint32_t vertex_input_state_manager::binding_index(graphics::vertex_layout const &vertex_layout)
    {
        auto &&[binding_description, attribute_descriptions] = vertex_input_state(vertex_layout);

        return binding_description.binding_index;
    }

    graphics::vertex_input_state const &vertex_input_state_manager::vertex_input_state(graphics::vertex_layout const &vertex_layout)
    {
        if (vertex_input_states_.count(vertex_layout) != 0)
            return vertex_input_states_.at(vertex_layout);

        auto const binding_index = static_cast<std::uint32_t>(std::size(vertex_input_states_));

        auto &&[size_in_bytes, attributes] = vertex_layout;

        graphics::vertex_input_binding binding_description{
            binding_index, static_cast<std::uint32_t>(size_in_bytes), graphics::VERTEX_INPUT_RATE::PER_VERTEX
        };

        std::vector<graphics::vertex_input_attribute> attribute_descriptions;

        std::transform(std::cbegin(attributes), std::cend(attributes), std::back_inserter(attribute_descriptions), [binding_index] (auto &&attribute)
        {
            auto format = graphics::get_vertex_attribute_format(attribute);

            auto location_index = graphics::get_vertex_attribute_semantic_index(attribute);

            return graphics::vertex_input_attribute{
                location_index, binding_index, static_cast<std::uint32_t>(attribute.offset_in_bytes), format
            };
        });

        vertex_input_states_.emplace(vertex_layout, graphics::vertex_input_state{ binding_description, attribute_descriptions });

        return vertex_input_states_.at(vertex_layout);
    }

    graphics::vertex_input_state vertex_input_state_manager::get_adjusted_vertex_input_state(
        graphics::vertex_layout const &vertex_layout, graphics::vertex_layout const &required_vertex_layout) const
    {
        if (vertex_input_states_.count(vertex_layout) == 0)
            throw std::runtime_error("failed to find vertex layout"s);

        auto &&vertex_input_state = vertex_input_states_.at(vertex_layout);

        graphics::vertex_input_state adjusted_vertex_input_state;
        adjusted_vertex_input_state.binding_description = vertex_input_state.binding_description;

        for (auto &&required_attribute : required_vertex_layout.attributes) {
            auto it = std::find_if(std::cbegin(vertex_input_state.attribute_descriptions), std::cend(vertex_input_state.attribute_descriptions),
                                   [&required_attribute] (auto &&attribute_description)
            {
                auto format = graphics::get_vertex_attribute_format(required_attribute);

                auto location_index = graphics::get_vertex_attribute_semantic_index(required_attribute);

                if (location_index != attribute_description.location_index)
                    return false;

                if (format != attribute_description.format)
                    return false;

                return true;
            });

            if (it == std::cend(vertex_input_state.attribute_descriptions))
                throw std::runtime_error("original vertex layout doesn't have required vertex attribute."s);

            adjusted_vertex_input_state.attribute_descriptions.push_back(*it);
        }

        return adjusted_vertex_input_state;
    }
}
