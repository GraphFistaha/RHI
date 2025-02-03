#include "Transferer.hpp"

#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

Transferer::Transferer(Context & ctx)
  : ContextualObject(ctx)
  , m_queueFamilyIndex(ctx.GetQueue(QueueType::Transfer).first)
  , m_queue(ctx.GetQueue(QueueType::Transfer).second)
  , m_writingTransferData(ctx, m_queue, m_queueFamilyIndex)
  , m_executingTransferData(ctx, m_queue, m_queueFamilyIndex)
{
  m_writingTransferData.submitter.BeginWriting();
}

Transferer::Transferer(Transferer && rhs) noexcept
  : ContextualObject(std::move(rhs))
  , m_writingTransferData(std::move(rhs.m_writingTransferData))
  , m_executingTransferData(std::move(rhs.m_executingTransferData))
{
  std::swap(m_queue, rhs.m_queue);
  std::swap(m_queueFamilyIndex, rhs.m_queueFamilyIndex);
}

Transferer & Transferer::operator=(Transferer && rhs) noexcept
{
  if (this != &rhs)
  {
    ContextualObject::operator=(std::move(rhs));
    std::swap(m_queue, rhs.m_queue);
    std::swap(m_queueFamilyIndex, rhs.m_queueFamilyIndex);
    std::swap(m_writingTransferData, rhs.m_writingTransferData);
    std::swap(m_executingTransferData, rhs.m_executingTransferData);
  }
  return *this;
}

IAwaitable * Transferer::DoTransfer()
{
  m_writingTransferData.submitter.EndWriting();
  std::swap(m_executingTransferData, m_writingTransferData);
  auto * awaitable = m_executingTransferData.submitter.Submit(true, {});
  m_writingTransferData.submitter.WaitForSubmitCompleted();
  m_writingTransferData.upload_data.clear();
  for (auto && [stagingBuffer, promise] : m_writingTransferData.download_data)
  {
    std::vector<uint8_t> tmp;
    tmp.resize(stagingBuffer.Size());
    if (auto map_ptr = stagingBuffer.Map())
    {
      std::memcpy(tmp.data(), map_ptr.get(), stagingBuffer.Size());
    }
    promise.set_value(std::move(tmp));
  }
  m_writingTransferData.download_data.clear();
  m_writingTransferData.submitter.Reset();
  m_writingTransferData.submitter.BeginWriting();
  return awaitable;
}

void Transferer::UploadBuffer(BufferGPU * dstBuffer, const uint8_t * srcData, size_t size,
                              size_t offset = 0) noexcept
{
  BufferGPU stagingBuffer(GetContext(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  stagingBuffer.UploadSync(srcData, size, offset);

  VkBufferCopy copy{};
  copy.dstOffset = 0;
  copy.srcOffset = 0;
  copy.size = stagingBuffer.Size();
  m_writingTransferData.submitter.PushCommand(vkCmdCopyBuffer, stagingBuffer.GetHandle(),
                                              dstBuffer->GetHandle(), 1, &copy);
}

void Transferer::UploadImage(ImageBase * dstImage, BufferGPU && stagingBuffer) noexcept
{
  auto && stagBuffer = m_writingTransferData.upload_data.emplace_back(std::move(stagingBuffer));
  VkImageLayout oldLayout = dstImage->GetLayout();
  dstImage->SetImageLayout(m_writingTransferData.submitter, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  VkBufferImageCopy region{};
  {
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageExtent = dstImage->GetVulkanExtent();
    region.imageOffset = {0, 0, 0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
  }

  m_writingTransferData.submitter.PushCommand(vkCmdCopyBufferToImage, stagBuffer.GetHandle(),
                                              dstImage->GetHandle(),
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  dstImage->SetImageLayout(m_writingTransferData.submitter, oldLayout);
}

std::future<std::vector<uint8_t>> Transferer::DownloadImage(
  ImageBase * srcImage, const CopyImageArguments & args) noexcept
{
  auto && downloadData =
    m_writingTransferData.download_data.emplace_back(std::promise<std::vector<uint8_t>>(),
                                                     BufferGPU(GetContext(), srcImage->Size(),
                                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT));
  VkImageLayout oldLayout = srcImage->GetLayout();
  srcImage->SetImageLayout(m_writingTransferData.submitter, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  VkBufferImageCopy region{};
  {
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageExtent = srcImage->GetVulkanExtent();
    region.imageOffset = {0, 0, 0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
  }

  m_writingTransferData.submitter.PushCommand(vkCmdCopyImageToBuffer, srcImage->GetHandle(),
                                              srcImage->GetLayout(), downloadData.first.GetHandle(),
                                              1, &region);
  srcImage->SetImageLayout(m_writingTransferData.submitter, oldLayout);
  return downloadData.second.get_future();
}

Transferer::TransferData::TransferData(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex)
  : submitter(ctx, queue, queueFamilyIndex, VK_PIPELINE_STAGE_TRANSFER_BIT)
{
}

} // namespace RHI::vulkan
