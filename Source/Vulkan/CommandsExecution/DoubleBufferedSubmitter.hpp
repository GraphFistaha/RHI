#pragma once
#include <CommandsExecution/Submitter.hpp>

namespace RHI::vulkan
{
struct DoubleBufferedSubmitter final
{
  explicit DoubleBufferedSubmitter(Context & ctx, QueueType type, VkPipelineStageFlags waitStages);

  details::CommandBuffer & GetWritingBuffer() & noexcept { return m_writingBuffer; }
  IAwaitable * Submit(bool waitPrevSubmitOnGPU, std::vector<VkSemaphore> && waitSemaphores);
  void WaitForSubmitCompleted();

private:
  details::Submitter m_writingBuffer;
  details::Submitter m_executingBuffer;
};
} // namespace RHI::vulkan
