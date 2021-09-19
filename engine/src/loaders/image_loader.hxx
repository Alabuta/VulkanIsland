#pragma once

#include <memory>
#include <string_view>


namespace resource
{
    struct texture;
    class resource_manager;
}

[[nodiscard]] std::shared_ptr<resource::texture>
load_texture(vulkan::device &device, resource::resource_manager &resource_manager, std::string_view name, VkCommandPool transfer_command_pool);
