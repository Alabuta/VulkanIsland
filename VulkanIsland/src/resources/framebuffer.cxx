
#include "framebuffer.hxx"


namespace resource
{
    std::size_t hash<resource::framebuffer_invariant>::operator() (resource::framebuffer_invariant const &invariant) const
    {
        std::size_t seed = 0;

        // TODO:: implement
    #if NOT_YET_IMPLEMENTED
        graphics::hash<graphics::render_pass> constexpr render_pass_hasher;
        boost::hash_combine(seed, render_pass_hasher(*invariant.render_pass));

        graphics::hash<resource::image_view> constexpr image_view_hasher;

        for (auto &&attachment : invariant.attachments)
            boost::hash_combine(seed, image_view_hasher(*attachment));
    #endif

        boost::hash_combine(seed, invariant.render_pass->handle());

        boost::hash_combine(seed, invariant.extent.width);
        boost::hash_combine(seed, invariant.extent.height);

        for (auto &&attachment : invariant.attachments)
            boost::hash_combine(seed, attachment->handle());

        return seed;
    }
}
