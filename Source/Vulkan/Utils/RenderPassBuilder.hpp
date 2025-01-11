#pragma once
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct RenderPassBuilder final
{
  void AddAttachment(const VkAttachmentDescription & description);
  void AddSubpass(const VkSubpassDescription & description);

  VkRenderPass Make(const VkDevice & device) const;
  void Reset();

private:
  std::vector<VkSubpassDescription> m_subpassDescriptions;
  std::vector<VkAttachmentDescription> m_attachments;
};
} // namespace RHI::vulkan::utils
