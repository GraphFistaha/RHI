#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>
#include "CommandBuffer.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan::details
{
/// @brief Submits commands into queue, owns primary command buffer
struct Submitter : public CommandBuffer
{
  explicit Submitter(const Context & ctx, vk::Queue queue, uint32_t queueFamily);
  virtual ~Submitter();

  VkSemaphore Submit(const std::vector<SemaphoreHandle> & waitSemaphores);
  void WaitForSubmitCompleted();

protected:
  uint32_t m_queueFamily;
  vk::Queue m_queue;
  /// semaphore to wait for commands have been completed on gpu
  vk::Semaphore m_commandsCompletedSemaphore;
  vk::Fence m_commandsCompletedFence; ///< fence to wait for commands have been completed on cpu
};

} // namespace RHI::vulkan::details
