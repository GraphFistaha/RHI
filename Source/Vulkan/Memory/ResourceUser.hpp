#pragma once
#include <variant>

#include <Memory/BufferInterface.hpp>
#include <Memory/TextureInterface.hpp>

namespace RHI::vulkan
{
using ResourcePtr = std::variant<IInternalBuffer *, IInternalTexture *>;
using ResourceUsageInfo =
  std::tuple<ResourcePtr, VkPipelineStageFlags2, VkAccessFlags2, VkImageLayout>;

struct IResourceUser
{
  virtual ~IResourceUser() = default;
  virtual void CollectResources(std::vector<ResourcePtr> & resources) const = 0;
  virtual void SynchroniseResources(details::CommandBuffer & commands) const = 0;
};
} // namespace RHI::vulkan
