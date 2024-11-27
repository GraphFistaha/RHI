#include "Transferer.hpp"

#include "../Resources/BufferGPU.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

Transferer::Transferer(const Context & ctx)
  : m_context(ctx)
{
  auto [familyIndex, queue] = ctx.GetQueue(QueueType::Transfer);
  m_submitter =
    std::make_unique<details::Submitter>(ctx, queue, familyIndex, VK_PIPELINE_STAGE_TRANSFER_BIT);
}

SemaphoreHandle Transferer::Flush()
{
  if (m_tasks.empty())
    return VK_NULL_HANDLE;
  m_submitter->WaitForSubmitCompleted();
  m_submitter->Reset();
  m_submitter->BeginWriting();
  while (!m_tasks.empty())
  {
    UploadTask task = std::move(m_tasks.front());
    m_tasks.pop();
    auto && dstBuffer = std::get<0>(task);
    auto && dstImage = std::get<1>(task);
    auto && stagingBuffer = std::get<2>(task);

    if (dstBuffer)
    {
      VkBufferCopy copy{};
      copy.dstOffset = 0;
      copy.srcOffset = 0;
      copy.size = stagingBuffer.Size();
      m_submitter->PushCommand(vkCmdCopyBuffer, stagingBuffer.GetHandle(), dstBuffer, 1, &copy);
    }
    else if (dstImage)
    {
      // TODO make upload of image
      //vkCmdCopyBufferToImage(m_submitter->GetCommandBuffer(),);
    }
  }
  m_submitter->EndWriting();
  return m_submitter->Submit(true, {});
}
void Transferer::UploadBuffer(VkBuffer dstBuffer, BufferGPU && stagingBuffer) noexcept
{
  m_tasks.emplace(UploadTask{dstBuffer, VK_NULL_HANDLE, std::move(stagingBuffer)});
}

void Transferer::UploadImage(VkImage dstImage, BufferGPU && stagingBuffer) noexcept
{
}

} // namespace RHI::vulkan
