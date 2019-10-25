#include "framebuffer.hxx"


namespace graphics
{
    std::size_t hash<resource::framebuffer>::operator() (resource::framebuffer const &framebuffer) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, framebuffer.width);
        boost::hash_combine(seed, framebuffer.height);

        // TODO:: implement
    #if NOT_YET_IMPLEMENTED
        graphics::hash<graphics::render_pass> constexpr render_pass_hasher;
        boost::hash_combine(seed, render_pass_hasher(*framebuffer.render_pass));

        graphics::hash<VulkanImage> constexpr image_hasher;

        for (auto &&attachment : framebuffer.attachments)
            boost::hash_combine(seed, image_hasher(*attachment));
    #endif

        return seed;
    }
}
