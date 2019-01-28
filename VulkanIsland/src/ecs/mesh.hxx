#pragma once

#include <limits>

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"
#include "resources/buffer.hxx"

#include "ecs.hxx"


namespace ecs
{
struct mesh final {
    std::shared_ptr<VulkanBuffer> vertexBuffer{nullptr};
    std::shared_ptr<VulkanBuffer> indexBuffer{nullptr};

    ;

    std::uint32_t vertexCount{0};
    std::uint32_t indexCount{0};

    std::uint32_t instanceCount{0};

    std::uint32_t firstIndex{0};
};

class MeshSystem final : public System {
public:

    MeshSystem(entity_registry &registry) noexcept : registry{registry} { }
    ~MeshSystem() = default;

    void update() override
    {
        auto view = registry.view<mesh>();

        view.each([this, &view] (auto &&node, auto &&transform)
        {
            ;
        });
    }

private:
    entity_registry &registry;
};
}
