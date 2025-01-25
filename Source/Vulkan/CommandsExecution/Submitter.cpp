#include "Submitter.hpp"

#include "../VulkanContext.hpp"

namespace RHI::vulkan::details
{
Submitter::Submitter(const Context & ctx, VkQueue queue, uint32_t queueFamily,
                     VkPipelineStageFlags waitStages)
  : CommandBuffer(ctx, queueFamily, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
  , m_queueFamily(queueFamily)
  , m_queue(queue)
  , m_waitStages(waitStages)
{
  std::array<Barrier *, 2> barriers{&m_oldBarrier, &m_newBarrier};
  for (auto && barrier : barriers)
  {
    barrier->first = utils::CreateVkSemaphore(ctx.GetDevice());
    barrier->second = utils::CreateFence(ctx.GetDevice(), true);
  }
  vkResetFences(m_context.GetDevice(), 1, &m_newBarrier.second);
}

Submitter::~Submitter()
{
  std::array<Barrier *, 2> barriers{&m_oldBarrier, &m_newBarrier};
  for (auto && barrier : barriers)
  {
    m_context.GetGarbageCollector().PushVkObjectToDestroy(barrier->first, nullptr);
    m_context.GetGarbageCollector().PushVkObjectToDestroy(barrier->second, nullptr);
  }
}

Barrier Submitter::Submit(bool waitPrevSubmitOnGPU, std::vector<VkSemaphore> && waitSemaphores)
{
  const VkSemaphore signalSem = m_newBarrier.first;
  const VkCommandBuffer buffer = GetHandle();

  if (!m_isFirstSubmit && waitPrevSubmitOnGPU)
    waitSemaphores.push_back(m_oldBarrier.first);
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

  auto res = vkQueueSubmit(m_queue, 1, &submitInfo, m_newBarrier.second);
  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to submit command buffer!");
  std::swap(m_oldBarrier, m_newBarrier);
  m_isFirstSubmit = false;
  return m_oldBarrier;
}

void Submitter::WaitForSubmitCompleted()
{
  const VkFence fence = m_oldBarrier.second;
  vkWaitForFences(m_context.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(m_context.GetDevice(), 1, &fence);
}

} // namespace RHI::vulkan::details
