#include "graphics_pipeline.hxx"


namespace graphics
{
    std::shared_ptr<graphics::pipeline> pipeline_manager::create_pipeline(std::shared_ptr<graphics::material> material)
    {
        if (pipelines_.count(material) == 0) {
            ;
        }

        return pipelines_.at(material);
    }
}

namespace graphics
{
    std::uint32_t vertex_input_state_manager::binding_index(graphics::vertex_layout const &vertex_layout)
    {
        auto &&[binding_description, attribute_descriptions] = vertex_input_state(vertex_layout);

        return binding_description.binding_index;
    }

    graphics::vertex_input_state const &vertex_input_state_manager::vertex_input_state(graphics::vertex_layout const &vertex_layout)
    {
        if (vertex_input_states_.count(vertex_layout) != 0)
            return vertex_input_states_.at(vertex_layout);

        auto const binding_index = static_cast<std::uint32_t>(std::size(vertex_input_states_));

        auto &&[size_in_bytes, attributes] = vertex_layout;

        graphics::vertex_input_binding binding_description{
            binding_index, static_cast<std::uint32_t>(size_in_bytes), vertex::INPUT_RATE::VERTEX
        };

        std::vector<graphics::vertex_input_attribute> attribute_descriptions;

        std::transform(std::cbegin(attributes), std::cend(attributes), std::back_inserter(attribute_descriptions), [binding_index] (auto &&attribute)
        {
            auto format = std::visit([normalized = attribute.normalized](auto &&type)
            {
                using T = std::remove_cvref_t<decltype(type)>;
                return getFormat<T::number, typename T::type>(normalized);

            }, attribute.type);

            auto location = std::visit([] (auto semantic)
            {
                using S = std::remove_cvref_t<decltype(semantic)>;
                return S::index;

            }, attribute.semantic);

            return graphics::vertex_input_attribute{
                location, binding_index, format, static_cast<std::uint32_t>(attribute.offset_in_bytes)
            };
        });
    }
}
