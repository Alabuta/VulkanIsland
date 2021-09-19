#include <boost/functional/hash.hpp>

#include "pipeline_states.hxx"


namespace graphics
{
    std::size_t hash<graphics::vertex_input_state>::operator() (graphics::vertex_input_state const &state) const
    {
        std::size_t seed = 0;

        graphics::hash<graphics::vertex_input_binding> constexpr binding_hasher;

        boost::hash_combine(seed, binding_hasher(state.binding_description));

        /*for (auto &&binding_description : state.binding_descriptions)
            boost::hash_combine(seed, binding_hasher(binding_description));*/

        graphics::hash<graphics::vertex_input_attribute> constexpr attribute_hasher;

        for (auto &&attribute_description : state.attribute_descriptions)
            boost::hash_combine(seed, attribute_hasher(attribute_description));

        return seed;
    }

    std::size_t hash<graphics::rasterization_state>::operator() (graphics::rasterization_state const &state) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, state.cull_mode);
        boost::hash_combine(seed, state.front_face);
        boost::hash_combine(seed, state.polygon_mode);
        boost::hash_combine(seed, state.line_width);

        return seed;
    }

    std::size_t hash<graphics::stencil_state>::operator() (graphics::stencil_state const &state) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, state.fail);
        boost::hash_combine(seed, state.pass);
        boost::hash_combine(seed, state.depth_fail);

        boost::hash_combine(seed, state.compare_operation);

        boost::hash_combine(seed, state.compare_mask);
        boost::hash_combine(seed, state.write_mask);
        boost::hash_combine(seed, state.reference);

        return seed;
    }

    std::size_t hash<graphics::depth_stencil_state>::operator() (graphics::depth_stencil_state const &state) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, state.depth_test_enable);
        boost::hash_combine(seed, state.depth_write_enable);
        boost::hash_combine(seed, state.depth_compare_operation);

        boost::hash_combine(seed, state.depth_bounds_test_enable);
        boost::hash_combine(seed, state.depth_bounds);

        boost::hash_combine(seed, state.stencil_test_enable);

        graphics::hash<graphics::stencil_state> constexpr stencil_state_hasher;

        boost::hash_combine(seed, stencil_state_hasher(state.front_stencil_state));
        boost::hash_combine(seed, stencil_state_hasher(state.back_stencil_state));

        return seed;
    }

    std::size_t hash<graphics::color_blend_attachment_state>::operator() (graphics::color_blend_attachment_state const &state) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, state.blend_enable);
        boost::hash_combine(seed, state.src_color_blend_factor);
        boost::hash_combine(seed, state.dst_color_blend_factor);
        boost::hash_combine(seed, state.color_blend_operation);
        boost::hash_combine(seed, state.src_alpha_blend_factor);
        boost::hash_combine(seed, state.dst_alpha_blend_factor);
        boost::hash_combine(seed, state.alpha_blend_operation);
        boost::hash_combine(seed, state.color_write_mask);

        return seed;
    }

    std::size_t hash<graphics::color_blend_state>::operator() (graphics::color_blend_state const &state) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, state.logic_operation_enable);
        boost::hash_combine(seed, state.logic_operation);
        boost::hash_combine(seed, state.blend_constants);

        graphics::hash<graphics::color_blend_attachment_state> constexpr hasher;

        for (auto &&attachment : state.attachment_states)
            boost::hash_combine(seed, hasher(attachment));

        return seed;
    }

    std::size_t hash<graphics::pipeline_states>::operator() (graphics::pipeline_states const &states) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, states.primitive_topology);

        graphics::hash<graphics::vertex_input_state> constexpr vertex_input_state_hasher;
        boost::hash_combine(seed, vertex_input_state_hasher(states.vertex_input_state));

        graphics::hash<graphics::rasterization_state> constexpr rasterization_state_hasher;
        boost::hash_combine(seed, rasterization_state_hasher(states.rasterization_state));

        graphics::hash<graphics::depth_stencil_state> constexpr depth_stencil_state_hasher;
        boost::hash_combine(seed, depth_stencil_state_hasher(states.depth_stencil_state));

        graphics::hash<graphics::color_blend_state> constexpr color_blend_state_hasher;
        boost::hash_combine(seed, color_blend_state_hasher(states.color_blend_state));

        return seed;
    }
}
