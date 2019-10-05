#include <boost/functional/hash.hpp>

#include "descriptors.hxx"


namespace graphics
{
    std::size_t hash<graphics::descriptor_set_binding>::operator() (graphics::descriptor_set_binding const &binding) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, binding.binding_index);
        boost::hash_combine(seed, binding.descriptor_count);
        boost::hash_combine(seed, binding.descriptor_type);
        boost::hash_combine(seed, binding.shader_stages);

        return seed;
    }
}

