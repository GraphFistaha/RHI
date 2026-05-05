#include "DoubleBufferedSubmitter.hpp"

namespace RHI::vulkan
{
BufferedSubmitter::BufferedSubmitter(Context & ctx, QueueType type, uint32_t buffersCount,
                                     VkPipelineStageFlags waitStages)

{
  m_submitters.reserve(buffersCount);
  for (uint32_t i = 0; i < buffersCount; ++i)
    m_submitters.emplace_back(ctx, type, waitStages);
  GetWritingBuffer().BeginWriting();
}

void BufferedSubmitter::WaitForSubmitCompleted()
{
  if (m_executingSubmitterIdx != -1)
    m_submitters[m_executingSubmitterIdx].WaitForSubmitCompleted();
}

AsyncTask * BufferedSubmitter::Submit(bool waitPrevSubmitOnGPU,
                                      std::vector<VkSemaphore> && waitSemaphores)
{
  if (m_submitters[m_writingSubmitterIdx].IsEmpty())
  {
    m_executingSubmitterIdx = -1;
    return nullptr;
  }
  m_submitters[m_writingSubmitterIdx].EndWriting();
  AsyncTask * result =
    m_submitters[m_writingSubmitterIdx].Submit(waitPrevSubmitOnGPU, std::move(waitSemaphores));
  m_executingSubmitterIdx =
    std::exchange(m_writingSubmitterIdx,
                  static_cast<uint32_t>((m_writingSubmitterIdx + 1u) % m_submitters.size()));
  m_submitters[m_writingSubmitterIdx].WaitForSubmitCompleted();
  m_submitters[m_writingSubmitterIdx].Reset();
  m_submitters[m_writingSubmitterIdx].BeginWriting();
  return result;
}
} // namespace RHI::vulkan
