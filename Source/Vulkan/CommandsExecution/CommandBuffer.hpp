#pragma once
#include <numeric>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}


namespace RHI::vulkan::details
{
struct CommandBuffer : public OwnedBy<Context>
{
  explicit CommandBuffer(Context & ctx, uint32_t queue_family, VkCommandBufferLevel level);
  virtual ~CommandBuffer();
  CommandBuffer(CommandBuffer && rhs) noexcept;
  CommandBuffer & operator=(CommandBuffer && rhs) noexcept;

  void BeginWriting() const;
  void BeginWriting(VkRenderPass renderPass, uint32_t subpassIndex,
                    VkFramebuffer framebuffer = VK_NULL_HANDLE) const;
  void EndWriting() const;
  void Reset();
  void AddCommands(const std::vector<VkCommandBuffer> & buffers);

  template<typename VkCmdFunc, typename... Args>
  void PushCommand(VkCmdFunc && func, Args &&... args) noexcept
  {
    func(m_buffer, std::forward<Args>(args)...);
    m_commandsCount++;
  }

  bool IsEmpty() const noexcept { return m_commandsCount == 0; }

public:
  VkCommandBuffer GetHandle() const noexcept { return m_buffer; }
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

private:
  VkCommandBufferLevel m_level;
  VkCommandPool m_pool = VK_NULL_HANDLE;
  VkCommandBuffer m_buffer = VK_NULL_HANDLE;
  size_t m_commandsCount = 0;
};

template<typename InIt>
void AccumulateCommands(CommandBuffer & dst, InIt begin, InIt end)
{
  std::vector<VkCommandBuffer> buffers;
  std::transform(begin, end, std::back_inserter(buffers), [](const CommandBuffer & buf)
                 { return static_cast<VkCommandBuffer>(buf.GetHandle()); });
  dst.AddCommands(buffers);
}

} // namespace RHI::vulkan::details
