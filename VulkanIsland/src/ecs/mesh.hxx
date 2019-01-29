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

    std::uint32_t vertexCount{0};
    std::uint32_t indexCount{0};

    std::uint32_t instanceCount{0};

    std::uint32_t firstIndex{0};

    template<class T1, class T2, typename std::enable_if_t<are_same_v<mesh, T1, T2>>...>
    bool constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
    {
        if (lhs.vertexBuffer == rhs.vertexBuffer)
            return lhs.firstIndex < rhs.firstIndex;

        return lhs.vertexBuffer < rhs.vertexBuffer;
    }
};


class MeshSystem final : public System {
public:

    MeshSystem(entity_registry &registry) noexcept : registry{registry} { }
    ~MeshSystem() = default;

    void update() override
    {
        auto view = registry.view<mesh>();

        view.each([this, &view] (auto &&)
        {
            ;
        });
    }

private:
    entity_registry &registry;
};
}
