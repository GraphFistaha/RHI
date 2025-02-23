#include "Transferer.hpp"

#include "../Images/ImageFormatsConversation.hpp"
#include "../Images/InternalImageTraits.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

Transferer::Transferer(Context & ctx)
  : OwnedBy<Context>(ctx)
  , m_queueFamilyIndex(ctx.GetQueue(QueueType::Transfer).first)
  , m_queue(ctx.GetQueue(QueueType::Transfer).second)
  , m_writingTransferData(ctx, m_queue, m_queueFamilyIndex)
  , m_executingTransferData(ctx, m_queue, m_queueFamilyIndex)
{
  m_writingTransferData.submitter.BeginWriting();
}

Transferer::Transferer(Transferer && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
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
    OwnedBy<Context>::operator=(std::move(rhs));
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

  // process upload data
  for (auto && [stagingBuffer, promise] : m_writingTransferData.upload_data)
  {
    UploadResult result = stagingBuffer.Size();
    promise.set_value(result);
  }
  m_writingTransferData.upload_data.clear();

  // process download data
  for (auto && [stagingBuffer, promise, createDownloadResult] : m_writingTransferData.download_data)
  {
    DownloadResult result = createDownloadResult(stagingBuffer);
    promise.set_value(std::move(result));
  }
  m_writingTransferData.download_data.clear();

  m_writingTransferData.submitter.Reset();
  m_writingTransferData.submitter.BeginWriting();
  return awaitable;
}

std::future<UploadResult> Transferer::UploadBuffer(BufferGPU & dstBuffer, const uint8_t * srcData,
                                                   size_t size, size_t offset)
{
  std::promise<UploadResult> promise;
  BufferGPU stagingBuffer(GetContext(), size - offset, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  stagingBuffer.UploadSync(srcData, size, offset);

  VkBufferCopy copy{};
  copy.dstOffset = 0;
  copy.srcOffset = 0;
  copy.size = stagingBuffer.Size();
  m_writingTransferData.submitter.PushCommand(vkCmdCopyBuffer, stagingBuffer.GetHandle(),
                                              dstBuffer.GetHandle(), 1, &copy);
  auto && data =
    m_writingTransferData.upload_data.emplace_back(std::move(stagingBuffer), std::move(promise));
  return data.second.get_future();
}

std::future<DownloadResult> Transferer::DownloadBuffer(BufferGPU & srcBuffer, size_t size,
                                                       size_t offset)
{
  std::promise<DownloadResult> promise;
  BufferGPU stagingBuffer(GetContext(), size - offset, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  VkBufferCopy copy{};
  copy.dstOffset = 0;
  copy.srcOffset = offset;
  copy.size = size;
  m_writingTransferData.submitter.PushCommand(vkCmdCopyBuffer, srcBuffer.GetHandle(),
                                              stagingBuffer.GetHandle(), 1, &copy);
  auto createDownloadResult = [](BufferGPU & stagingBuffer) -> DownloadResult
  {
    DownloadResult result(stagingBuffer.Size(), 0);
    if (auto scopedPtr = stagingBuffer.Map())
      std::memcpy(result.data(), scopedPtr.get(), stagingBuffer.Size());
    return result;
  };
  auto && data = m_writingTransferData.download_data.emplace_back(std::move(stagingBuffer),
                                                                  std::move(promise),
                                                                  std::move(createDownloadResult));
  return std::get<1>(data).get_future();
}

std::future<UploadResult> Transferer::UploadImage(ImageBase & dstImage, const uint8_t * srcData,
                                                  const CopyImageArguments & args)
{
  std::promise<UploadResult> promise;
  BufferGPU stagingBuffer(GetContext(), dstImage.Size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  if (auto && mapped_ptr = stagingBuffer.Map())
  {
    CopyImageFromHost(srcData, args.src.extent, args.src, args.hostFormat,
                      reinterpret_cast<uint8_t *>(mapped_ptr.get()), args.dst.extent, args.dst,
                      dstImage.GetVulkanFormat());
    mapped_ptr.reset();
    stagingBuffer.Flush();
  }
  else
  {
    throw std::runtime_error("Failed to fill staging buffer");
  }

  VkBufferImageCopy region{};
  {
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageExtent = {args.dst.extent[0], args.dst.extent[1], args.dst.extent[2]};
    region.imageOffset = {static_cast<int>(args.dst.offset[0]),
                          static_cast<int>(args.dst.offset[1]),
                          static_cast<int>(args.dst.offset[2])};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
  }

  VkImageLayout oldLayout = dstImage.GetLayout();
  dstImage.TransferLayout(m_writingTransferData.submitter, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  m_writingTransferData.submitter.PushCommand(vkCmdCopyBufferToImage, stagingBuffer.GetHandle(),
                                              dstImage.GetHandle(),
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  auto && data =
    m_writingTransferData.upload_data.emplace_back(std::move(stagingBuffer), std::move(promise));
  dstImage.TransferLayout(m_writingTransferData.submitter, oldLayout);
  return data.second.get_future();
}

std::future<DownloadResult> Transferer::DownloadImage(ImageBase & srcImage, HostImageFormat format,
                                                      const ImageRegion & imgRegion)
{
  std::promise<DownloadResult> promise;
  BufferGPU stagingBuffer(GetContext(),
                          RHI::utils::GetSizeOfImage(imgRegion.extent, srcImage.GetVulkanFormat()),
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  VkBufferImageCopy region{};
  {
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageExtent = {imgRegion.extent[0], imgRegion.extent[1], imgRegion.extent[2]};
    region.imageOffset = {static_cast<int>(imgRegion.offset[0]),
                          static_cast<int>(imgRegion.offset[1]),
                          static_cast<int>(imgRegion.offset[2])};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
  }

  auto createDownloadResult = [imgRegion, format, srcFormat = srcImage.GetVulkanFormat()](
                                BufferGPU & stagingBuffer) -> DownloadResult
  {
    DownloadResult result(RHI::utils::GetSizeOfImage(imgRegion.extent, format));
    if (auto scopedPtr = stagingBuffer.Map())
    {
      ImageRegion rgn{{0, 0, 0}, imgRegion.extent};
      CopyImageToHost(reinterpret_cast<uint8_t *>(scopedPtr.get()), imgRegion.extent, rgn,
                      srcFormat, result.data(), imgRegion.extent, rgn, format);
    }
    return result;
  };

  VkImageLayout oldLayout = srcImage.GetLayout();
  srcImage.TransferLayout(m_writingTransferData.submitter, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  m_writingTransferData.submitter.PushCommand(vkCmdCopyImageToBuffer, srcImage.GetHandle(),
                                              srcImage.GetLayout(), stagingBuffer.GetHandle(), 1,
                                              &region);
  auto && data = m_writingTransferData.download_data.emplace_back(std::move(stagingBuffer),
                                                                  std::move(promise),
                                                                  std::move(createDownloadResult));
  srcImage.TransferLayout(m_writingTransferData.submitter, oldLayout);
  return std::get<1>(data).get_future();
}

Transferer::TransferData::TransferData(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex)
  : submitter(ctx, queue, queueFamilyIndex, VK_PIPELINE_STAGE_TRANSFER_BIT)
{
}

} // namespace RHI::vulkan
