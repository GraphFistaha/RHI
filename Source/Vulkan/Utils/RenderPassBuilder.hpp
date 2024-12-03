#pragma once
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan::utils
{
struct RenderPassBuilder final
{
  using SubpassSlots = std::vector<std::pair<ShaderImageSlot, uint32_t>>;
  void AddAttachment(const VkAttachmentDescription & description);
  void AddSubpass(SubpassSlots slotsLayout);

  vk::RenderPass Make(const vk::Device & device) const;
  void Reset();

private:
  std::vector<SubpassSlots> m_slots;
  std::vector<VkAttachmentDescription> m_attachments;
};
} // namespace RHI::vulkan::utils
