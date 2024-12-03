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
  std::queue<BufferGPU> buffers;
  while (!m_tasks.empty())
  {
    UploadTask task = std::move(m_tasks.front());
    m_tasks.pop();
    auto && dstBuffer = std::get<0>(task);
    ImageGPU * dstImage = std::get<1>(task);
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
      dstImage->SetImageLayout(*m_submitter , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      auto && args = dstImage->GetParameters();
      VkBufferImageCopy region{};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;

      region.imageExtent = {args.width, args.height, args.depth};
      region.imageOffset = {0, 0, 0};

      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount = 1;

      m_submitter->PushCommand(vkCmdCopyBufferToImage, stagingBuffer.GetHandle(),
                               dstImage->GetHandle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
      dstImage->SetImageLayout(*m_submitter, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    
    buffers.push(std::move(stagingBuffer));
  }
  m_submitter->EndWriting();
  return m_submitter->Submit(true, {});
}

void Transferer::UploadBuffer(VkBuffer dstBuffer, BufferGPU && stagingBuffer) noexcept
{
  m_tasks.emplace(UploadTask{dstBuffer, VK_NULL_HANDLE, std::move(stagingBuffer)});
}

void Transferer::UploadImage(ImageGPU * dstImage, BufferGPU && stagingBuffer) noexcept
{
  m_tasks.emplace(UploadTask{VK_NULL_HANDLE, dstImage, std::move(stagingBuffer)});
}

} // namespace RHI::vulkan
