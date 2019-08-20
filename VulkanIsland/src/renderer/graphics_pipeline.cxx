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
