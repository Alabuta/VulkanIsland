#pragma once

#include <limits>

#include "main.hxx"
#include "utility/helpers.hxx"
#include "math/math.hxx"
#include "resources/buffer.hxx"
#include "resources/resource.hxx"

#include "ecs.hxx"


#if NOT_YET_IMPLEMENTED
namespace ecs
{
struct mesh final {
    std::optional<resource::vertex_buffer> vertexBuffer;
    std::optional<resource::index_buffer> indexBuffer;

    std::uint32_t vertexCount{0};
    std::uint32_t indexCount{0};

    template<class T1, class T2, typename std::enable_if_t<are_same_v<mesh, T1, T2>>* = nullptr>
    bool constexpr operator() (T1 &&lhs, T2 &&rhs) const noexcept
    {
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
#endif
