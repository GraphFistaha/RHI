#include "Submitter.hpp"

#include "../VulkanContext.hpp"

namespace RHI::vulkan::details
{
Submitter::Submitter(const Context & ctx, vk::Queue queue, uint32_t queueFamily)
  : CommandBuffer(ctx, queueFamily, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
  , m_queueFamily(queueFamily)
  , m_queue(queue)
{
  m_commandsCompletedFence = utils::CreateFence(ctx.GetDevice(), true);
  m_commandsCompletedSemaphore = utils::CreateVkSemaphore(ctx.GetDevice());
}

Submitter::~Submitter()
{
  if (!!m_commandsCompletedFence)
    vkDestroySemaphore(m_context.GetDevice(), m_commandsCompletedSemaphore, nullptr);
  if (!!m_commandsCompletedFence)
    vkDestroyFence(m_context.GetDevice(), m_commandsCompletedFence, nullptr);
}

VkSemaphore Submitter::Submit(const std::vector<SemaphoreHandle> & waitSemaphores)
{
  const VkSemaphore signalSem = m_commandsCompletedSemaphore;
  const VkCommandBuffer buffer = GetHandle();
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  submitInfo.pWaitSemaphores = reinterpret_cast<const VkSemaphore *>(waitSemaphores.data());
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &buffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &signalSem;

  auto res = vkQueueSubmit(m_queue, 1, &submitInfo, m_commandsCompletedFence);
  if (res != VK_SUCCESS)
    throw std::runtime_error("failed to submit command buffer!");

  return m_commandsCompletedSemaphore;
}

void Submitter::WaitForSubmitCompleted()
{
  const VkFence fence = m_commandsCompletedFence;
  vkWaitForFences(m_context.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(m_context.GetDevice(), 1, &fence);
}

} // namespace RHI::vulkan::details
