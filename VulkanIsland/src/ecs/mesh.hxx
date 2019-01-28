#pragma once

#include <limits>

#include "main.hxx"
#include "helpers.hxx"
#include "math.hxx"
#include "recources/buffer.hxx"

#include "ecs.hxx"


namespace ecs
{
struct mesh final {
    std::shared_ptr<VulkanBuffer> vertexBuffer{nullptr};
    std::shared_ptr<VulkanBuffer> indexBuffer{nullptr};
    std::uint32_t vertexCount;
};
}
