#pragma once

#include <string>
#include <string_view>
#include <optional>

#include "entityx/entityx.h"
namespace ex = entityx;

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"

struct Transform final {
    glm::mat4 localMatrix;
    glm::mat4 worldMatrix;

    template<class T1, class T2, std::enable_if_t<are_same_v<glm::mat4, T1, T2>>...>
    Transform(T1 &&localMatrix, T2 &&worldMatrix) : localMatrix{std::forward<T1>(localMatrix)}, worldMatrix{std::forward<T2>(worldMatrix)} {}
};

template<class T, std::enable_if_t<std::is_same_v<std::decay_t<T>, glm::mat4>>...>
inline std::ostream &operator<< (std::ostream &stream, T &&m)
{
    stream << std::setprecision(4) << std::fixed;

    stream << m[0][0] << ' ' << m[0][1] << ' ' << m[0][2] << ' ' << m[0][3] << '\n';
    stream << m[1][0] << ' ' << m[1][1] << ' ' << m[1][2] << ' ' << m[1][3] << '\n';
    stream << m[2][0] << ' ' << m[2][1] << ' ' << m[2][2] << ' ' << m[2][3] << '\n';
    stream << m[3][0] << ' ' << m[3][1] << ' ' << m[3][2] << ' ' << m[3][3];

    return stream;
}

struct TransformSytem final : public ex::System<Transform> {
    void update(ex::EntityManager &es, ex::EventManager &events, ex::TimeDelta dt) final
    {
        es.each<Transform>([] (auto &&entity, auto &&transform)
        {
            std::cout << entity << "\n" << transform.localMatrix << "\n\n" << transform.worldMatrix << "\n\n";
        });
    }
};

using node_index_t = std::size_t;
auto constexpr kINVALID_INDEX{std::numeric_limits<node_index_t>::max()};

enum class NodeHandle : node_index_t {
    nINVALID_HANDLE = kINVALID_INDEX
};

struct Node final {
    node_index_t depth{kINVALID_INDEX};
    node_index_t offset{kINVALID_INDEX};

    constexpr Node(node_index_t depth, node_index_t offset) : depth{depth}, offset{offset} { }

    Node() = default;
};

struct NodeInfo final {
    NodeHandle parent{NodeHandle::nINVALID_HANDLE};
    NodeHandle handle{NodeHandle::nINVALID_HANDLE};

    ex::Entity entity;

    std::string name;

    struct ChildrenRange final {
        node_index_t begin{0}, end{0};
    } children;

    NodeInfo(NodeHandle parent, NodeHandle handle, ex::Entity entity, std::string_view name) : parent{parent}, handle{handle}, entity{entity}, name{name} { }

    NodeInfo() = default;
};

class SceneTree final {
public:

    SceneTree(std::string_view name = "noname"sv) : name{name}
    {
        entityX = std::make_unique<ex::EntityX>();

        entityX->systems.add<TransformSytem>();
        entityX->systems.configure();

        nodes.emplace_back(0, 0);
        layers.emplace_back(1, NodeInfo{NodeHandle::nINVALID_HANDLE, root(), entityX->entities.create(), name});
    }

    bool isNodeHandleValid(NodeHandle handle) const noexcept { return handle != NodeHandle::nINVALID_HANDLE; }

    constexpr NodeHandle root() const noexcept { return static_cast<NodeHandle>(0); }

    std::optional<NodeHandle> AddChild(NodeHandle parentHandle);
    void DestroyNode(NodeHandle handle);

    std::optional<NodeHandle> AttachNode(NodeHandle parentHandle, std::string_view name = "noname"sv);

    void RemoveNode(NodeHandle handle);
    void DestroyChildren(NodeHandle handle);

    void SetName(NodeHandle handle, std::string_view name);

    template<class T, class... Ts>
    void AddComponent(NodeHandle handle, Ts &&...args)
    {
        if (!isNodeHandleValid(handle))
            return;

        auto &&node = nodes.at(static_cast<std::size_t>(handle));

        if (!isNodeValid(node))
            return;

        auto &&layer = layers.at(node.depth);
        auto &&info = layer.at(node.offset);

        info.entity.assign<T>(std::forward<Ts>(args)...);
    }

    void Update();

private:
    std::unique_ptr<ex::EntityX> entityX;

    std::string name;

    std::vector<Node> nodes;

    using layer_t = std::vector<NodeInfo>;
    std::vector<layer_t> layers;

    bool isNodeValid(Node node) const noexcept { return node.depth != kINVALID_INDEX && node.offset != kINVALID_INDEX; }

#if 0
    struct chunk_t final {
        node_index_t begin{0}, end{0};
        node_index_t size{0};

        chunk_t() = default;

        chunk_t(node_index_t begin, node_index_t end) : begin{begin}, end{end}, size{end - begin} { }

        struct comparator final {
            using is_transparent = void;

            template<class L, class R>
            std::enable_if_t<are_same_v<chunk_t, L, R>, bool>
                operator() (L &&lhs, R &&rhs) const noexcept
            {
                return lhs.size < rhs.size;
            }

            template<class T, class S>
            std::enable_if_t<std::is_same_v<chunk_t, std::decay_t<T>> && std::is_integral_v<S>, bool>
                operator() (T &&chunk, S size) const noexcept
            {
                return chunk.size < size;
            }

            template<class S, class T>
            std::enable_if_t<std::is_same_v<chunk_t, std::decay_t<T>> && std::is_integral_v<S>, bool>
                operator() (S size, T &&chunk) const noexcept
            {
                return chunk.size < size;
            }
        };
    };
#endif

    using chunks_t = std::set<node_index_t>;
    std::vector<chunks_t> layersChunks;
};