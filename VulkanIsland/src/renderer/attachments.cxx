#include <boost/functional/hash.hpp>

#include "attachments.hxx"


namespace graphics
{
    std::size_t hash<graphics::attachment_description>::operator() (graphics::attachment_description const &attachment_description) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, attachment_description.format);
        boost::hash_combine(seed, attachment_description.samples_count);
        boost::hash_combine(seed, attachment_description.load_op);
        boost::hash_combine(seed, attachment_description.store_op);
        boost::hash_combine(seed, attachment_description.initial_layout);
        boost::hash_combine(seed, attachment_description.final_layout);

        return seed;
    }

    std::size_t hash<graphics::attachment_reference>::operator() (graphics::attachment_reference const &attachment_reference) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, attachment_reference.pass_index);
        boost::hash_combine(seed, attachment_reference.attachment_index);
        boost::hash_combine(seed, attachment_reference.subpass_layout);

        return seed;
    }
}
