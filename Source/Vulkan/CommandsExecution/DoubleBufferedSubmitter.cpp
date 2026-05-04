#include "DoubleBufferedSubmitter.hpp"

namespace RHI::vulkan
{
DoubleBufferedSubmitter::DoubleBufferedSubmitter(Context & ctx, QueueType type,
                                                 VkPipelineStageFlags waitStages)
  : m_writingBuffer(ctx, type, waitStages)
  , m_executingBuffer(ctx, type, waitStages)
{
  m_writingBuffer.BeginWriting();
}

void DoubleBufferedSubmitter::WaitForSubmitCompleted()
{
  m_executingBuffer.WaitForSubmitCompleted();
}

IAwaitable * DoubleBufferedSubmitter::Submit(bool waitPrevSubmitOnGPU,
                                             std::vector<VkSemaphore> && waitSemaphores)
{
  if (m_writingBuffer.IsEmpty())
    return nullptr;
  m_writingBuffer.EndWriting();
  IAwaitable * result = m_writingBuffer.Submit(waitPrevSubmitOnGPU, std::move(waitSemaphores));
  std::swap(m_writingBuffer, m_executingBuffer);
  m_writingBuffer.WaitForSubmitCompleted();
  m_writingBuffer.Reset();
  m_writingBuffer.BeginWriting();
  return result;
}
} // namespace RHI::vulkan
