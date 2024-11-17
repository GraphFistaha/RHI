#include "Submitter.hpp"

namespace RHI::vulkan::details
{
Submitter::Submitter(const Context & ctx, vk::Queue queue, uint32_t queueFamily)
  : m_context(ctx)
  , m_queueFamily(queueFamily)
  , m_queue(queue)
  , m_buffer(ctx, m_queueFamily, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
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

void Submitter::BeginWrite()
{
  WaitForSubmitCompleted();
  m_buffer.BeginWriting();
}

void Submitter::EndWrite()
{
  m_buffer.EndWriting();
}

void Submitter::Clear()
{
  m_buffer.Reset();
}

VkSemaphore Submitter::Submit(const std::vector<SemaphoreHandle> & waitSemaphores)
{
  const VkSemaphore signalSem = m_commandsCompletedSemaphore;
  const VkCommandBuffer buffer = m_buffer.GetHandle();
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = waitSemaphores.size();
  submitInfo.pWaitSemaphores = reinterpret_cast<const VkSemaphore*>(waitSemaphores.data());
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
