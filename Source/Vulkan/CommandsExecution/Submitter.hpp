#pragma once

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>
#include "CommandBuffer.hpp"

namespace RHI::vulkan
{
enum class QueueType : uint8_t;
struct Context;
}

namespace RHI::vulkan::details
{
/// @brief Submits commands into queue, owns primary command buffer
struct Submitter : public CommandBuffer
{
  explicit Submitter(const Context & ctx, VkQueue queue, uint32_t queueFamily,
                     VkPipelineStageFlags waitStages);
  virtual ~Submitter();

  VkSemaphore Submit(bool waitPrevSubmitOnGPU, std::vector<SemaphoreHandle> && waitSemaphores);
  void WaitForSubmitCompleted();

protected:
  VkPipelineStageFlags m_waitStages;
  uint32_t m_queueFamily;
  VkQueue m_queue;

  /// semaphore to wait for commands have been completed on gpu
  /// fence to wait for commands have been completed on cpu
  using Barrier = std::pair<VkSemaphore, VkFence>;
  Barrier m_oldBarrier;
  Barrier m_newBarrier;
  bool m_isFirstSubmit = true;
};

} // namespace RHI::vulkan::details
