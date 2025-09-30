#include "Submitter.hpp"

#include <VulkanContext.hpp>

namespace RHI::vulkan::details
{
Submitter::Submitter(Context & ctx, VkQueue queue, uint32_t queueFamily,
                     VkPipelineStageFlags waitStages)
  : CommandBuffer(ctx, queueFamily, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
  , m_queueFamily(queueFamily)
  , m_queue(queue)
  , m_waitStages(waitStages)
  , m_newBarrier(ctx)
  , m_oldBarrier(ctx)
{
}

Submitter::Submitter(Submitter && rhs) noexcept
  : CommandBuffer(std::move(rhs))
  , m_newBarrier(std::move(rhs.m_newBarrier))
  , m_oldBarrier(std::move(rhs.m_oldBarrier))
{
  std::swap(m_waitStages, rhs.m_waitStages);
  std::swap(m_queue, rhs.m_queue);
  std::swap(m_queueFamily, rhs.m_queueFamily);
  std::swap(m_isFirstSubmit, rhs.m_isFirstSubmit);
}

Submitter & Submitter::operator=(Submitter && rhs) noexcept
{
  if (this != &rhs)
  {
    CommandBuffer::operator=(std::move(rhs));
    std::swap(m_waitStages, rhs.m_waitStages);
    std::swap(m_newBarrier, rhs.m_newBarrier);
    std::swap(m_oldBarrier, rhs.m_oldBarrier);
    std::swap(m_queue, rhs.m_queue);
    std::swap(m_queueFamily, rhs.m_queueFamily);
    std::swap(m_isFirstSubmit, rhs.m_isFirstSubmit);
  }
  return *this;
}

AsyncTask * Submitter::Submit(bool waitPrevSubmitOnGPU, std::vector<VkSemaphore> && waitSemaphores)
{
  const VkSemaphore signalSem = m_newBarrier.GetSemaphore();
  const VkCommandBuffer buffer = GetHandle();

  if (!m_isFirstSubmit && waitPrevSubmitOnGPU)
    waitSemaphores.push_back(m_oldBarrier.GetSemaphore());

  assert(std::all_of(waitSemaphores.begin(), waitSemaphores.end(),
                     [](VkSemaphore sem) { return !!sem; }));

  std::vector<VkPipelineStageFlags> waitStages(waitSemaphores.size(), m_waitStages);
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  submitInfo.pWaitSemaphores = reinterpret_cast<const VkSemaphore *>(waitSemaphores.data());
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &buffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &signalSem;

  m_newBarrier.StartTask();
  auto res = vkQueueSubmit(m_queue, 1, &submitInfo, m_newBarrier.GetFence());
  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to submit command buffer!");
  std::swap(m_oldBarrier, m_newBarrier);
  m_isFirstSubmit = false;
  waitSemaphores.clear();
  return &m_oldBarrier;
}

void Submitter::WaitForSubmitCompleted()
{
  m_oldBarrier.Wait();
}

} // namespace RHI::vulkan::details
