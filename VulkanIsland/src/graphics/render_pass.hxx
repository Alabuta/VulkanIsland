#pragma once

#include <unordered_set>
#include <optional>
#include <vector>
#include <memory>
#include <set>

#include "vulkan/device.hxx"
#include "graphics.hxx"
#include "attachments.hxx"


namespace graphics
{
    struct subpass_dependency final {
        std::optional<std::uint32_t> source_index;
        std::optional<std::uint32_t> destination_index;

        graphics::PIPELINE_STAGE source_stage;
        graphics::PIPELINE_STAGE destination_stage;

        graphics::MEMORY_ACCESS_TYPE source_access;
        graphics::MEMORY_ACCESS_TYPE destination_access;
    };

    struct subpass_description final {
        std::set<graphics::attachment_reference> input_attachments;
        std::set<graphics::attachment_reference> color_attachments;
        std::set<graphics::attachment_reference> resolve_attachments;

        std::optional<graphics::attachment_reference> depth_stencil_attachment;

        std::vector<std::uint32_t> preserve_attachments;
    };
}

namespace graphics
{
    class render_pass final {
    public:

        render_pass(VkRenderPass handle) noexcept : handle_{handle} { }

        VkRenderPass handle() const noexcept { return handle_; }

    private:

        VkRenderPass handle_;
    };
}

namespace graphics
{
    class render_pass_manager final {
    public:

        render_pass_manager(vulkan::device const &device) noexcept : device_{device} { }

        [[nodiscard]] std::shared_ptr<graphics::render_pass>
        create_render_pass(std::vector<graphics::attachment_description> const &attachment_descriptions,
                           std::vector<graphics::subpass_description> const &subpass_descriptions,
                           std::vector<graphics::subpass_dependency> const &subpass_dependencies);

    private:

        vulkan::device const &device_;
    };
}
