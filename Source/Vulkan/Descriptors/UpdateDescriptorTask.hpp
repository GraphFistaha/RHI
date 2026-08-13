#pragma once
#include <functional>
#include <span>

#include <vulkan/vulkan.h>
namespace RHI::vulkan
{
struct Context;
using UpdateDescriptorTask = std::function<void(const Context &, std::span<const VkDescriptorSet>)>;
} // namespace RHI::vulkan
