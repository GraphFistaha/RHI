#pragma once
#include <numeric>

#include "../VulkanContext.hpp"

namespace RHI::vulkan::details
{
struct CommandBuffer final
{
  explicit CommandBuffer(const Context & ctx, uint32_t queue_family, VkCommandBufferLevel level);
  ~CommandBuffer();

  void BeginWriting() const;
  void BeginWriting(VkRenderPass renderPass, uint32_t subpassIndex, VkFramebuffer framebuffer = VK_NULL_HANDLE) const;
  void EndWriting() const;
  void Reset();
  void AddCommands(const std::vector<VkCommandBuffer> & buffers);

public:
  vk::CommandBuffer GetHandle() const noexcept { return m_buffer; }

private:
  const Context & m_context;
  VkCommandBufferLevel m_level;
  vk::CommandPool m_pool = VK_NULL_HANDLE;
  vk::CommandBuffer m_buffer = VK_NULL_HANDLE;
};

template<typename InIt>
void AccumulateCommands(CommandBuffer & dst, InIt begin, InIt end)
{
  std::vector<VkCommandBuffer> buffers;
  std::transform(begin, end, std::back_inserter(buffers), [](const CommandBuffer & buf)
                 { return static_cast<VkCommandBuffer>(buf.GetHandle()); });
  dst.AddCommands(buffers);
}

} // namespace RHI::vulkan
