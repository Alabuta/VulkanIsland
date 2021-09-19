#include <iostream>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>

#include "utility/exceptions.hxx"
#include "loaders/SPIRV_loader.hxx"
#include "shader_program.hxx"


namespace graphics
{
    std::shared_ptr<graphics::shader_module> shader_manager::shader_module(std::string_view name)
    {
        if (shader_modules_.contains(std::string{name}))
            return shader_modules_.at(std::string{name});

        else return create_shader_module(name);
    }

    std::shared_ptr<graphics::shader_module> shader_manager::create_shader_module(std::string_view name)
    {
        // TODO:: move to loader class.
        auto const shader_byte_code = loader::load_SPIRV(name);

        if (shader_byte_code.empty())
            throw resource::exception(fmt::format("failed to open shader file: {}"s, name));

        std::shared_ptr<graphics::shader_module> shader_module;

        if (std::size(shader_byte_code) % sizeof(std::uint32_t) != 0)
            throw resource::exception(fmt::format("invalid byte code buffer size: {}"s, name));

        else {
            VkShaderModuleCreateInfo const create_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                nullptr, 0,
                std::size(shader_byte_code),
                reinterpret_cast<std::uint32_t const *>(std::data(shader_byte_code))
            };

            VkShaderModule handle;

            if (auto result = vkCreateShaderModule(device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
                throw vulkan::exception(fmt::format("failed to create shader module: {0:#x}"s, result));

            else {
                shader_module.reset(new graphics::shader_module{handle}, [this] (graphics::shader_module *ptr_module)
                    {
                        vkDestroyShaderModule(device_.handle(), ptr_module->handle(), nullptr);

                        delete ptr_module;
                    }
                );

                shader_modules_.emplace(name, shader_module);
            }
        }

        return shader_module;
    }
}

namespace graphics
{
    std::size_t hash<graphics::specialization_constant>::operator() (graphics::specialization_constant const &specialization_constant) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, specialization_constant.id);
        boost::hash_combine(seed, specialization_constant.value);

        return seed;
    }

    std::size_t hash<graphics::shader_stage>::operator() (graphics::shader_stage const &stage) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, stage.module_name);
        boost::hash_combine(seed, stage.technique_index);
        boost::hash_combine(seed, stage.semantic);

        for (auto [id, value] : stage.constants) {
            boost::hash_combine(seed, id);
            boost::hash_combine(seed, value);
        }

        return seed;
    }
}
