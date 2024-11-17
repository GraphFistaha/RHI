#pragma once

#include "../VulkanContext.hpp"
#include "CommandBuffer.hpp"

namespace RHI::vulkan::details
{
/// @brief Submits commands into queue, owns primary command buffer
struct Submitter
{
  explicit Submitter(const Context & ctx, vk::Queue queue, uint32_t queueFamily);
  virtual ~Submitter();

  void BeginWrite();
  void EndWrite();
  VkSemaphore Submit(const std::vector<SemaphoreHandle> & waitSemaphores);
  void WaitForSubmitCompleted();

  vk::CommandBuffer GetCommandBuffer() const noexcept { return m_buffer.GetHandle(); }

protected:
  const Context & m_context;
  uint32_t m_queueFamily;
  vk::Queue m_queue;
  CommandBuffer m_buffer;
  /// semaphore to wait for commands have been completed on gpu
  vk::Semaphore m_commandsCompletedSemaphore;
  vk::Fence m_commandsCompletedFence; ///< fence to wait for commands have been completed on cpu
};

} // namespace RHI::vulkan::details
