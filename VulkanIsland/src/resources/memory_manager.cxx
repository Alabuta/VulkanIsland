#include <unordered_map>
#include <iostream>
#include <vector>
#include <set>

#include <fmt/format.h>
#include <boost/functional/hash.hpp>

#include "graphics/graphics_api.hxx"

#include "buffer.hxx"
#include "image.hxx"

#include "resource_manager.hxx"
#include "memory_manager.hxx"


namespace
{
    std::optional<std::uint32_t>
    find_memory_type_index(vulkan::device const &device, std::uint32_t filter, graphics::MEMORY_PROPERTY_TYPE memory_property_types) noexcept
    {
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(device.physical_handle(), &memory_properties);

        auto const memory_types = mpl::to_array(memory_properties.memoryTypes);

        auto it_type = std::find_if(std::cbegin(memory_types), std::cend(memory_types), [filter, memory_property_types, i = 0u] (auto type) mutable
        {
            auto const property_flags = convert_to::vulkan(memory_property_types);

            return (filter & (1u << i++)) && (type.propertyFlags & property_flags) == property_flags;
        });

        if (it_type < std::next(std::cbegin(memory_types), memory_properties.memoryTypeCount))
            return static_cast<std::uint32_t>(std::distance(std::cbegin(memory_types), it_type));

        return { };
    }
}

namespace resource
{
    struct memory_chunk final {
        memory_chunk(std::size_t offset, std::size_t size) noexcept : offset{offset}, size{size} { }

        std::size_t offset{0}, size{0};

        struct comparator final {
            using is_transparent = void;

            template<class L, class R, typename std::enable_if_t<mpl::are_same_v<memory_chunk, L, R>>* = nullptr>
            bool operator() (L &&lhs, R &&rhs) const noexcept
            {
                return lhs.size < rhs.size;
            }

            template<class T, class S, typename std::enable_if_t<mpl::are_same_v<memory_chunk, std::remove_cvref_t<T>> && std::is_integral_v<S>>* = nullptr>
            bool operator() (T &&chunk, S size) const noexcept
            {
                return chunk.size < size;
            }
                
            template<class S, class T, typename std::enable_if_t<mpl::are_same_v<memory_chunk, std::remove_cvref_t<T>> && std::is_integral_v<S>>* = nullptr>
            bool operator() (S size, T &&chunk) const noexcept
            {
                return chunk.size < size;
            }
        };
    };

    struct memory_block final {
        memory_block(std::size_t available_size) : available_size{available_size}, avaiable_chunks{{0, available_size}} { }

        std::size_t available_size{0};

        std::multiset<resource::memory_chunk, resource::memory_chunk::comparator> avaiable_chunks;
    };

    struct memory_pool final {
        memory_pool(std::uint32_t memory_type_index, graphics::MEMORY_PROPERTY_TYPE properties, bool linear)
            : properties{properties}, type_index{memory_type_index}, linear{linear} { }

        graphics::MEMORY_PROPERTY_TYPE properties;

        std::uint32_t type_index{0};
        std::size_t allocated_size{0};

        bool linear;

        //std::unordered_map<resource::device_memory, resource::memory_block> memory_blocks;
        std::unordered_map<VkDeviceMemory, resource::memory_block> memory_blocks;

        template<class T, typename std::enable_if_t<mpl::are_same_v<std::remove_cvref_t<T>, memory_pool>>* = nullptr>
        bool constexpr operator== (T &&rhs) const noexcept
        {
            return properties == rhs.properties && type_index == rhs.type_index && linear == rhs.linear;
        }
    };

    template<>
    struct hash<resource::memory_pool> {
        std::size_t operator() (resource::memory_pool const &memory_pool) const
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, memory_pool.properties);
            boost::hash_combine(seed, memory_pool.type_index);
            boost::hash_combine(seed, memory_pool.allocated_size);
            boost::hash_combine(seed, memory_pool.linear);

            return seed;
        }
    };

    struct memory_manager::memory_helper final {
        memory_helper(vulkan::device const &device) : device{device}
        {
            buffer_image_granularity = device.device_limits().buffer_image_granularity;

            if (kBLOCK_ALLOCATION_SIZE < buffer_image_granularity)
                throw std::runtime_error("default memory page size is less than buffer image granularity size"s);
        }

        vulkan::device const &device;

        std::size_t buffer_image_granularity{0};
        std::size_t total_allocated_size{0};

        std::unordered_map<std::size_t, resource::memory_pool> memory_pools_;

        std::shared_ptr<resource::device_memory>
        allocate_memory(VkMemoryRequirements &&memory_requirements, graphics::MEMORY_PROPERTY_TYPE, bool linear);

        void deallocate_memory(resource::device_memory &&device_memory);

        std::optional<decltype(memory_pool::memory_blocks)::iterator>
        allocate_memory_block(std::size_t size_in_bytes, std::uint32_t memory_type_index, graphics::MEMORY_PROPERTY_TYPE properties, bool linear);
    };

    std::shared_ptr<resource::device_memory>
    memory_manager::memory_helper::allocate_memory(VkMemoryRequirements &&memory_requirements, graphics::MEMORY_PROPERTY_TYPE memory_property_types, bool linear)
    {
        return std::shared_ptr<resource::device_memory>();
    }

    void memory_manager::memory_helper::deallocate_memory(resource::device_memory &&device_memory)
    {
        auto const key = ([&device_memory]
        {
            std::size_t seed = 0;

            boost::hash_combine(seed, device_memory.type_index());
            boost::hash_combine(seed, device_memory.properties());
            boost::hash_combine(seed, device_memory.linear());

            return seed;
        })();

        if (!memory_pools_.contains(key)) {
            std::cerr << "Memory manager: dead memory chunk encountered."s << std::endl;
            return;
        };

        auto &&memory_pool = memory_pools_.at(key);

        auto const memory_handle = device_memory.handle();
        auto const memory_size = device_memory.size();
        auto const memory_type_index = device_memory.type_index();

        if (!memory_pool.memory_blocks.contains(memory_handle)) {
            std::cerr << "Memory manager: dead memory chunk encountered."s << std::endl;
            return;
        }

        auto &&memory_block = memory_pool.memory_blocks.at(memory_handle);
        auto &&avaiable_chunks = memory_block.avaiable_chunks;

        std::cout << fmt::format("Memory type index #{0:#x} : releasing chunk {} KB.\n"s, memory_type_index, static_cast<float>(memory_size) / 1024.f);
    }
}

namespace resource
{
    memory_manager::memory_manager(vulkan::device const &device)
        : device_{device}, memory_helper_{std::make_unique<memory_manager::memory_helper>(device)}
    {
        ;
    }

    template<>
    std::shared_ptr<resource::device_memory>
    memory_manager::allocate_memory(std::shared_ptr<resource::buffer> buffer, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        auto constexpr linear_memory = true;

        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device_.handle(), buffer->handle(), &memory_requirements);

        return memory_helper_->allocate_memory(std::move(memory_requirements), memory_property_types, linear_memory);
    }

    template<>
    std::shared_ptr<resource::device_memory>
    memory_manager::allocate_memory(std::shared_ptr<resource::image> image, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        auto const linear_memory = image->tiling() == graphics::IMAGE_TILING::LINEAR;

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device_.handle(), image->handle(), &memory_requirements);

        return memory_helper_->allocate_memory(std::move(memory_requirements), memory_property_types, linear_memory);
    }

    /*std::size_t hash<resource::device_memory>::operator() (resource::device_memory const &device_memory) const
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, device_memory.type_index());
        boost::hash_combine(seed, device_memory.properties());
        boost::hash_combine(seed, device_memory.linear());

        return seed;
    }*/
}
