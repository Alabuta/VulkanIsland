#include <fmt/format.h>

#include <boost/uuid/name_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "loaders/SPIRV_loader.hxx"
#include "shader_program.hxx"


namespace graphics
{
    std::shared_ptr<graphics::shader_module> shader_manager::create_shader_module(std::string_view name, std::uint32_t technique_index)
    {
        auto const key = std::pair{std::string{name}, technique_index};

        if (modules_by_techinques_.count(key) != 0)
            return modules_by_techinques_.at(key);

        std::shared_ptr<graphics::shader_module> shader_module;

        auto full_name = fmt::format("{}.{}"s, name, technique_index);

        boost::uuids::name_generator_sha1 gen(boost::uuids::ns::dns());
        auto hashed_name = boost::uuids::to_string(gen(full_name));

        // TODO:: move to loader class.
        auto const shader_byte_code = loader::load_SPIRV(hashed_name);

        if (shader_byte_code.empty())
            throw std::runtime_error("failed to open vertex shader file"s);

        if (std::size(shader_byte_code) % sizeof(std::uint32_t) != 0)
            std::cerr << "invalid byte code buffer size\n"s;

        else {
            VkShaderModuleCreateInfo const create_info{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                nullptr, 0,
                std::size(shader_byte_code),
                reinterpret_cast<std::uint32_t const *>(std::data(shader_byte_code))
            };

            VkShaderModule handle;

            if (auto result = vkCreateShaderModule(vulkan_device_.handle(), &create_info, nullptr, &handle); result != VK_SUCCESS)
                std::cerr << fmt::format("failed to create shader module: {0:#x}\n"s, result);

            else shader_module.reset(
                new graphics::shader_module{handle},
                [this] (graphics::shader_module *ptr_module) {
                    vkDestroyShaderModule(vulkan_device_.handle(), ptr_module->handle(), nullptr);

                    delete ptr_module;
                }
            );
        }

        modules_by_techinques_.emplace(key, shader_module);

        return shader_module;
    }
}
