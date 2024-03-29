#include <unordered_map>
#include <iostream>
#include <ranges>
#include <vector>
#include <set>

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

#include <boost/align.hpp>
#include <boost/align/align.hpp>
#include <boost/functional/hash.hpp>

#include "utility/exceptions.hxx"
#include "graphics/graphics_api.hxx"

#include "buffer.hxx"
#include "image.hxx"

#include "resource_manager.hxx"
#include "memory_manager.hxx"


namespace
{
    std::size_t hash_memory_block_properties(std::uint32_t type_index, graphics::MEMORY_PROPERTY_TYPE properties, bool is_linear)
    {
        std::size_t seed = 0;

        boost::hash_combine(seed, type_index);
        boost::hash_combine(seed, properties);
        boost::hash_combine(seed, is_linear);

        return seed;
    }

    std::optional<std::uint32_t>
    find_memory_type_index(vulkan::device const &device, std::uint32_t filter, graphics::MEMORY_PROPERTY_TYPE memory_property_types) noexcept
    {
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(device.physical_handle(), &memory_properties);

        auto const memory_types = mpl::to_array(memory_properties.memoryTypes);

        auto it_type = std::ranges::find_if(memory_types, [filter, memory_property_types, i = 0u] (auto type) mutable
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

            template<class L, class R>
            requires mpl::are_same_v<memory_chunk, L, R>
            bool operator() (L lhs, R rhs) const noexcept
            {
                return lhs.size < rhs.size;
            }

            template<class T, class S>
            requires mpl::are_same_v<memory_chunk, T> && std::is_unsigned_v<S>
            bool operator() (T chunk, S chunk_size) const noexcept
            {
                return chunk.size < chunk_size;
            }
                
            template<class S, class T>
            requires mpl::are_same_v<memory_chunk, T> && std::is_unsigned_v<S>
            bool operator() (S chunk_size, T chunk) const noexcept
            {
                return chunk.size < chunk_size;
            }
        };
    };

    struct memory_page final {
        explicit memory_page(std::size_t available_size) : available_size{available_size}, available_chunks{{0, available_size}} { }

        std::size_t available_size{0};

        std::multiset<resource::memory_chunk, resource::memory_chunk::comparator> available_chunks;
    };

    struct memory_pool final {
        memory_pool(graphics::MEMORY_PROPERTY_TYPE properties, std::uint32_t memory_type_index, bool is_linear)
            : properties{properties}, type_index{memory_type_index}, is_linear{is_linear} { }

        graphics::MEMORY_PROPERTY_TYPE properties;

        std::uint32_t type_index{0};
        std::size_t allocated_size{0};

        bool is_linear;

        //std::unordered_map<resource::memory_block, resource::memory_page> memory_blocks;
        std::unordered_map<VkDeviceMemory, resource::memory_page> memory_blocks;

        template<class T>
        requires mpl::are_same_v<std::remove_cvref_t<T>, memory_pool>
        bool constexpr operator== (T &&rhs) const noexcept
        {
            return properties == rhs.properties && type_index == rhs.type_index && is_linear == rhs.is_linear;
        }
    };

    struct memory_allocator final {
        vulkan::device const &device;

        std::size_t buffer_image_granularity{0};
        std::size_t total_allocated_size{0};

        std::unordered_map<std::size_t, resource::memory_pool> memory_pools;

        explicit memory_allocator(vulkan::device const& device);
        ~memory_allocator();

        memory_allocator(memory_allocator const &) = default;
        memory_allocator(memory_allocator &&) = default;

        memory_allocator &operator= (memory_allocator const &) = delete;
        memory_allocator &operator= (memory_allocator &&) = delete;

        std::shared_ptr<resource::memory_block>
        allocate_memory(VkMemoryRequirements &&memory_requirements, graphics::MEMORY_PROPERTY_TYPE, bool is_linear);

        void deallocate_memory(resource::memory_block &&memory_block);

        decltype(memory_pool::memory_blocks)::iterator
        allocate_memory_block(std::size_t size_bytes, std::uint32_t memory_type_index, graphics::MEMORY_PROPERTY_TYPE properties, bool is_linear);
    };

    memory_allocator::memory_allocator(vulkan::device const& device) : device{ device }
    {
        buffer_image_granularity = device.device_limits().buffer_image_granularity;

        if (resource::memory_manager::kPAGE_ALLOCATION_SIZE < buffer_image_granularity)
            throw memory::bad_allocation("default memory page size is less than buffer image granularity size."s);
    }

    memory_allocator::~memory_allocator()
    {
        for (auto& memory_pool : memory_pools | std::views::values)
            for (const auto& memory_handle : memory_pool.memory_blocks | std::views::keys)
                vkFreeMemory(device.handle(), memory_handle, nullptr);

        memory_pools.clear();
    }

    std::shared_ptr<resource::memory_block>
    memory_allocator::allocate_memory(VkMemoryRequirements &&memory_requirements, graphics::MEMORY_PROPERTY_TYPE properties, bool is_linear)
    {
    #ifndef  _MSC_VER
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wuseless-cast"
    #endif
        auto const required_size = static_cast<std::size_t>(memory_requirements.size);
        auto const required_alignment = static_cast<std::size_t>(memory_requirements.alignment);
    #ifndef  _MSC_VER
        #pragma GCC diagnostic pop
    #endif

        if (required_size > resource::memory_manager::kPAGE_ALLOCATION_SIZE)
            throw memory::bad_allocation("requested allocation size is bigger than memory page size."s);

        std::uint32_t memory_type_index = 0;

        if (auto index = find_memory_type_index(device, memory_requirements.memoryTypeBits, properties); index)
            memory_type_index = *index;

        else throw memory::bad_allocation("failed to find suitable memory type."s);

        auto const key = hash_memory_block_properties(memory_type_index, properties, is_linear);

        if (!memory_pools.contains(key))
            memory_pools.try_emplace(key, properties, memory_type_index, is_linear);

        auto &&memory_pool = memory_pools.at(key);
        auto &&memory_blocks = memory_pool.memory_blocks;

        typename decltype(resource::memory_page::available_chunks)::iterator it_chunk;

        auto it_block = std::ranges::find_if(
        memory_blocks, [&it_chunk, required_size, required_alignment](auto&& pair)
        {
            auto&& [memory_handle, memory_page] = pair;

            if (memory_page.available_size < required_size)
                return false;

            auto&& available_chunks = memory_page.available_chunks;

            auto it_chunk_begin = available_chunks.lower_bound(required_size);
            auto it_chunk_end = available_chunks.upper_bound(required_size);

            it_chunk = std::find_if(it_chunk_begin, it_chunk_end, [required_size, required_alignment](auto&& chunk)
            {
                auto aligned_offset = boost::alignment::align_up(chunk.offset, required_alignment);

                /*if (is_linear)
                aligned_offset = boost::alignment::align_up(aligned_offset, image_granularity);*/

                return aligned_offset + required_size <= chunk.offset + chunk.size;
            });

            return it_chunk != it_chunk_end;
        });

        if (it_block == std::end(memory_blocks)) {
            it_block = allocate_memory_block(resource::memory_manager::kPAGE_ALLOCATION_SIZE, memory_type_index, properties, is_linear);

            auto &&available_chunks = it_block->second.available_chunks;

            it_chunk = available_chunks.lower_bound(resource::memory_manager::kPAGE_ALLOCATION_SIZE);

            if (it_chunk == std::end(available_chunks))
                throw memory::exception("failed to find available memory chunk."s);
        }

        auto &&memory_page = it_block->second;
        auto &&available_chunks = memory_page.available_chunks;

        if (auto node_handle = available_chunks.extract(it_chunk); node_handle) {
            auto &&[offset, size] = node_handle.value();

            auto aligned_offset = boost::alignment::align_up(offset, required_alignment);

            /*if (is_linear)
                aligned_offset = boost::alignment::align_up(aligned_offset, image_granularity);*/

            if (aligned_offset > offset)
                available_chunks.emplace(offset, aligned_offset - offset);

            size -= required_size + aligned_offset - offset;
            offset = aligned_offset + required_size;

            available_chunks.insert(std::move(node_handle));

            memory_page.available_size -= required_size;

            available_chunks.erase(resource::memory_chunk{0, 0});

            auto const kilobytes = static_cast<float>(required_size) / 1024.f;

            fmt::print("Memory manager: type index #{} : sub-allocation {} KB.\n", memory_type_index, kilobytes);

            return std::shared_ptr<resource::memory_block>{
                new resource::memory_block{it_block->first, required_size, aligned_offset, memory_type_index, properties, is_linear},
                                            [this] (resource::memory_block *const ptr_memory)
                {
                    deallocate_memory(std::forward<resource::memory_block>(*ptr_memory));

                    delete ptr_memory;
                }
            };
        }

        throw memory::exception("failed to find memory chunk for extraction"s);
    }

    void memory_allocator::deallocate_memory(resource::memory_block &&memory_block)
    {
        auto const key = hash_memory_block_properties(memory_block.type_index(), memory_block.properties(), memory_block.is_linear());

        if (!memory_pools.contains(key)) {
            std::cerr << "Memory manager: dead memory chunk encountered." << std::endl;
            return;
        }

        auto &&memory_pool = memory_pools.at(key);

        auto const memory_handle = memory_block.handle();
        auto const memory_size = memory_block.size();
        auto const memory_offset = memory_block.offset();
        auto const memory_type_index = memory_block.type_index();

        if (!memory_pool.memory_blocks.contains(memory_handle)) {
            std::cerr << "Memory manager: dead memory chunk encountered." << std::endl;
            return;
        }

        auto &&memory_page = memory_pool.memory_blocks.at(memory_handle);
        auto &&available_chunks = memory_page.available_chunks;

        fmt::print("Memory manager: type index #{} : releasing chunk {} KB.\n", memory_type_index, static_cast<float>(memory_size) / 1024.f);

        auto it_chunk = available_chunks.emplace(memory_offset, memory_size);

        auto find_adjacent_chunk = [] (auto begin, auto end, auto it)
        {
            return std::find_if(begin, end, [it] (auto &&chunk)
            {
                return chunk.offset + chunk.size == it->offset || it->offset + it->size == chunk.offset;
            });
        };

        auto it_adjacent_chunk = find_adjacent_chunk(std::begin(available_chunks), std::end(available_chunks), it_chunk);

        while (it_adjacent_chunk != std::end(available_chunks)) {
            auto [offset_a, size_a] = *it_chunk;
            auto [offset_b, size_b] = *it_adjacent_chunk;

            available_chunks.erase(it_chunk);
            available_chunks.erase(it_adjacent_chunk);

            it_chunk = available_chunks.emplace(std::min(offset_a, offset_b), size_a + size_b);

            it_adjacent_chunk = find_adjacent_chunk(std::begin(available_chunks), std::end(available_chunks), it_chunk);
        }

        memory_page.available_size += memory_size;
    }

    decltype(memory_pool::memory_blocks)::iterator
    memory_allocator::allocate_memory_block(std::size_t size_bytes, std::uint32_t memory_type_index, graphics::MEMORY_PROPERTY_TYPE properties, bool is_linear)
    {
        auto const key = hash_memory_block_properties(memory_type_index, properties, is_linear);

        if (!memory_pools.contains(key))
            throw memory::exception(fmt::format("failed to find instantiated memory pool for type index #{}", memory_type_index));

        auto &&memory_pool = memory_pools.at(key);
        auto &&memory_blocks = memory_pool.memory_blocks;

#if defined(__GNUC__) || defined(__GNUG__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
        VkMemoryAllocateInfo const allocation_info{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            nullptr,
            static_cast<VkDeviceSize>(size_bytes),
            memory_type_index
        };
#if defined(__GNUC__) || defined(__GNUG__)
    #pragma GCC diagnostic pop
#endif

        VkDeviceMemory handle;

        if (auto result = vkAllocateMemory(device.handle(), &allocation_info, nullptr, &handle); result != VK_SUCCESS)
            throw memory::bad_allocation("failed to allocate memory block from memory pool."s);

        total_allocated_size += size_bytes;
        memory_pool.allocated_size += size_bytes;

        auto it_memory_block = memory_blocks.try_emplace(handle, size_bytes).first;

        auto block_index = std::size(memory_blocks);

        auto kilobytes = static_cast<float>(size_bytes) / 1024.f;
        auto megabytes = static_cast<float>(total_allocated_size) / std::pow(2.f, 20.f);

        fmt::print("Memory manager: type index #{} : {}th page allocation {} KB/{} MB.\n", memory_type_index, block_index, kilobytes, megabytes);

        return it_memory_block;
    }
}

namespace resource
{
    memory_manager::memory_manager(vulkan::device const &device)
        : device_{device}, allocator_{std::make_shared<resource::memory_allocator>(device)} { }

    std::shared_ptr<resource::memory_block>
    memory_manager::allocate_buffer_memory(resource::buffer const &buffer, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        auto constexpr linear_memory = true;

        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device_.handle(), buffer.handle(), &memory_requirements);

        return allocator_->allocate_memory(std::move(memory_requirements), memory_property_types, linear_memory);
    }

    std::shared_ptr<resource::memory_block>
    memory_manager::allocate_image_memory(resource::image const &image, graphics::MEMORY_PROPERTY_TYPE memory_property_types)
    {
        auto const linear_memory = image.tiling() == graphics::IMAGE_TILING::LINEAR;

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device_.handle(), image.handle(), &memory_requirements);

        return allocator_->allocate_memory(std::move(memory_requirements), memory_property_types, linear_memory);
    }

    memory_block::memory_block(VkDeviceMemory handle, std::size_t size, std::size_t offset, std::uint32_t type_index,
                                 graphics::MEMORY_PROPERTY_TYPE properties, bool is_linear) noexcept
        : handle_{handle}, size_{size}, offset_{offset}, type_index_{type_index}, properties_{properties}, is_linear_{is_linear} { }
}
