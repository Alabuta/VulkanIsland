#pragma once

#include <iomanip>

#include "entityx/entityx.hh"
namespace ex = entityx;

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"



namespace semantic
{
enum class eSEMANTIC_INDEX : std::size_t {
    nPOSITION = 0,
    nNORMAL,
    nTEXCOORD_0,
    nTEXCOORD_1,
    nTANGENT,
    nCOLOR_0,
    nJOINTS_0,
    nWEIGHTS_0
};

template<eSEMANTIC_INDEX SI>
struct attribute {
    static auto constexpr semantic{SI};

    auto constexpr operator< (attribute rhs) const noexcept
    {
        return semantic < rhs.semantic;
    }
};

struct position : attribute<eSEMANTIC_INDEX::nPOSITION> { };
struct normal : attribute<eSEMANTIC_INDEX::nNORMAL> { };
struct tex_coord_0 : attribute<eSEMANTIC_INDEX::nTEXCOORD_0> { };
struct tex_coord_1 : attribute<eSEMANTIC_INDEX::nTEXCOORD_1> { };
struct tangent : attribute<eSEMANTIC_INDEX::nTANGENT> { };
struct color_0 : attribute<eSEMANTIC_INDEX::nCOLOR_0> { };
struct joints_0 : attribute<eSEMANTIC_INDEX::nJOINTS_0> { };
struct weights_0 : attribute<eSEMANTIC_INDEX::nWEIGHTS_0> { };
}

using semantic_t = std::variant<
    semantic::position,
    semantic::normal,
    semantic::tex_coord_0,
    semantic::tex_coord_1,
    semantic::tangent,
    semantic::color_0,
    semantic::joints_0,
    semantic::weights_0
>;

/*struct Mesh final {
    glm::mat4 localMatrix;
    glm::mat4 worldMatrix;

    template<class T1, class T2, std::enable_if_t<are_same_v<glm::mat4, T1, T2>>...>
    Mesh(T1 &&localMatrix, T2 &&worldMatrix) : localMatrix{std::forward<T1>(localMatrix)}, worldMatrix{std::forward<T2>(worldMatrix)} {}
};

struct MeshSytem final : public ex::System<Mesh> {
    void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) final
    {
        es.each<Mesh>([] (auto &&entity, auto &&mesh)
        {
            ;
        });
    }
};*/
