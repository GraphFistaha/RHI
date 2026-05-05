#pragma once
#include <CommandsExecution/Submitter.hpp>

namespace RHI::vulkan
{

struct BufferedSubmitter final
{
  explicit BufferedSubmitter(Context & ctx, QueueType type, uint32_t buffersCount,
                             VkPipelineStageFlags waitStages);

  details::CommandBuffer & GetWritingBuffer() & noexcept
  {
    return m_submitters[m_writingSubmitterIdx];
  }
  AsyncTask * Submit(bool waitPrevSubmitOnGPU, std::vector<VkSemaphore> && waitSemaphores);
  void WaitForSubmitCompleted();

private:
  std::vector<details::Submitter> m_submitters;
  uint32_t m_writingSubmitterIdx = 0;
  uint32_t m_executingSubmitterIdx = -1;
};
} // namespace RHI::vulkan
