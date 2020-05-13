#include <string>
using namespace std::string_literals;

#include <boost/functional/hash.hpp>

#include "vertex.hxx"


namespace graphics
{
    std::uint32_t get_vertex_attribute_semantic_index(graphics::vertex_attribute const &vertex_attribute)
    {
        return std::visit([] (auto semantic)
        {
            using S = std::remove_cvref_t<decltype(semantic)>;
            return S::index;

        }, vertex_attribute.semantic);
    }
}

namespace graphics
{
    std::size_t hash<graphics::vertex_attribute>::operator() (graphics::vertex_attribute const &attribute) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, attribute.semantic.index());
        boost::hash_combine(seed, attribute.format);
        boost::hash_combine(seed, attribute.offset_in_bytes);

        return seed;
    }

    std::size_t hash<graphics::vertex_layout>::operator() (graphics::vertex_layout const &layout) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, layout.size_in_bytes);

        graphics::hash<graphics::vertex_attribute> constexpr attribute_hasher;

        for (auto &&attribute : layout.attributes)
            boost::hash_combine(seed, attribute_hasher(attribute));

        return seed;
    }

    std::size_t hash<graphics::vertex_input_binding>::operator() (graphics::vertex_input_binding const &binding) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, binding.binding_index);
        boost::hash_combine(seed, binding.stride_in_bytes);
        boost::hash_combine(seed, binding.input_rate);

        return seed;
    }

    std::size_t hash<graphics::vertex_input_attribute>::operator() (graphics::vertex_input_attribute const &input_attribute) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, input_attribute.location_index);
        boost::hash_combine(seed, input_attribute.binding_index);
        boost::hash_combine(seed, input_attribute.offset_in_bytes);
        boost::hash_combine(seed, input_attribute.format);

        return seed;
    }
}
