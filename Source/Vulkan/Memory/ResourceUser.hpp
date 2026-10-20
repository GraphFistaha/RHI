#pragma once
#include <variant>

#include <Attachments/Attachment.hpp>
#include <Memory/BufferInterface.hpp>
#include <Memory/TextureInterface.hpp>


namespace RHI::vulkan
{
using ResourcePtr =
  std::variant<IInternalBuffer *, IInternalTexture *, uint32_t /*attachmentIndex*/>;

struct ResourceState final
{
  VkPipelineStageFlags2 currentStage = VK_PIPELINE_STAGE_2_NONE;
  VkAccessFlagBits2 requiredAccess = VK_ACCESS_2_NONE;
  VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};
using Barrier = std::pair<ResourcePtr, ResourceState /*requiresState*/>;

struct IResourceUser
{
  virtual ~IResourceUser() = default;
  virtual void CollectBarriers(std::vector<Barrier> & barriers) const = 0;
};
} // namespace RHI::vulkan
