#include <tuple>

#include <fmt/format.h>

#include "graphics_pipeline.hxx"


namespace convert_to
{
    [[nodiscard]] std::tuple<VkPipelineVertexInputStateCreateInfo, VkVertexInputBindingDescription, std::vector<VkVertexInputAttributeDescription>>
    vulkan(graphics::vertex_input_state const &vertex_input_state)
    {
        auto [binding_index, size_in_bytes, input_rate] = vertex_input_state.binding_description;

        VkVertexInputBindingDescription const binding_description{
            binding_index, size_in_bytes, convert_to::vulkan(input_rate)
        };

        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

        std::transform(std::cbegin(vertex_input_state.attribute_descriptions), std::cend(vertex_input_state.attribute_descriptions),
                       std::back_inserter(attribute_descriptions), [binding_index] (auto &&attribute_description)
        {
            auto [
                location_index, binding_index, offset_in_bytes, format
            ] = attribute_description;

            return VkVertexInputAttributeDescription{
                location_index, binding_index, convert_to::vulkan(format), offset_in_bytes
            };
        });

        VkPipelineVertexInputStateCreateInfo const info{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            nullptr, 0,
            1u, &binding_description,
            static_cast<std::uint32_t>(std::size(attribute_descriptions)), std::data(attribute_descriptions),
        };

        return std::tuple{info, binding_description, attribute_descriptions};
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

    [[nodiscard]] std::pair<VkPipelineColorBlendStateCreateInfo, std::vector<VkPipelineColorBlendAttachmentState>>
    vulkan(graphics::color_blend_state const &color_blend_state)
    {
        std::vector<VkPipelineColorBlendAttachmentState> attachment_states;

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

        return std::make_pair(create_info, attachment_states);
    }
}

namespace graphics
{
    std::shared_ptr<graphics::pipeline> pipeline_factory::create_pipeline(
        std::shared_ptr<graphics::material> material, graphics::pipeline_states const &pipeline_states,
        VkPipelineLayout layout, VkRenderPass render_pass, std::uint32_t subpass_index
    )
    {
        graphics::pipeline_invariant key{material, pipeline_states, layout, render_pass, subpass_index};

        if (pipelines_.count(key) != 0)
            return pipelines_.at(key);

        auto &&shader_stages = material->shader_stages;

        auto &&[
            primitive_topology, vertex_input_state,
            rasterization_state, depth_stencil_state, color_blend_state
        ] = pipeline_states;

        auto &&[vertex_input_state1, binding_description, attribute_descriptions] = convert_to::vulkan1(vertex_input_state);

        VkPipelineInputAssemblyStateCreateInfo const input_assembly_state{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            nullptr, 0,
            convert_to::vulkan(primitive_topology),
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

        VkPipelineMultisampleStateCreateInfo const multisample_state{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            nullptr, 0,
            convert_to::vulkan(1u), // vulkan_device_.samplesCount()
            VK_FALSE, 1,
            nullptr,
            VK_FALSE,
            VK_FALSE
        };

        VkGraphicsPipelineCreateInfo const create_info{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            nullptr,
            VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
            0, nullptr,//static_cast<std::uint32_t>(std::size(shader_stages)), std::data(shader_stages),
            &vertex_input_state1,
            &input_assembly_state,
            nullptr,
            &viewport_state,
            &convert_to::vulkan(rasterization_state),
            &multisample_state,
            &convert_to::vulkan(depth_stencil_state),
            &convert_to::vulkan(color_blend_state),
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
}
