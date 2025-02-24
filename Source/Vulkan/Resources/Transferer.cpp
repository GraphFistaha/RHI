#include "Transferer.hpp"

#include "../Images/ImageFormatsConversation.hpp"
#include "../Images/InternalImageTraits.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

namespace details
{
TransferSwapchain::TransferSwapchain(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex)
  : OwnedBy<Context>(ctx)
  , m_queueFamilyIndex(queueFamilyIndex)
  , m_queue(queue)
  , m_writingTransferBuffer(ctx, m_queue, m_queueFamilyIndex)
  , m_executingTransferBuffer(ctx, m_queue, m_queueFamilyIndex)
{
  m_writingTransferBuffer.submitter.BeginWriting();
}

TransferSwapchain::TransferSwapchain(TransferSwapchain && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
  , m_writingTransferBuffer(std::move(rhs.m_writingTransferBuffer))
  , m_executingTransferBuffer(std::move(rhs.m_executingTransferBuffer))
{
  std::swap(m_queue, rhs.m_queue);
  std::swap(m_queueFamilyIndex, rhs.m_queueFamilyIndex);
}

TransferSwapchain & TransferSwapchain::operator=(TransferSwapchain && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_queue, rhs.m_queue);
    std::swap(m_queueFamilyIndex, rhs.m_queueFamilyIndex);
    std::swap(m_writingTransferBuffer, rhs.m_writingTransferBuffer);
    std::swap(m_executingTransferBuffer, rhs.m_executingTransferBuffer);
  }
  return *this;
}

IAwaitable * TransferSwapchain::DoTransfer()
{
  m_writingTransferBuffer.submitter.EndWriting();
  std::swap(m_executingTransferBuffer, m_writingTransferBuffer);
  auto * awaitable = m_executingTransferBuffer.submitter.Submit(true, {});
  m_writingTransferBuffer.submitter.WaitForSubmitCompleted();

  // process upload data
  for (auto && [stagingBuffer, promise] : m_writingTransferBuffer.upload_data)
  {
    UploadResult result = stagingBuffer.Size();
    promise.set_value(result);
  }
  m_writingTransferBuffer.upload_data.clear();

  // process download data
  for (auto && [stagingBuffer, promise, createDownloadResult] :
       m_writingTransferBuffer.download_data)
  {
    DownloadResult result = createDownloadResult(stagingBuffer);
    promise.set_value(std::move(result));
  }
  m_writingTransferBuffer.download_data.clear();

  m_writingTransferBuffer.submitter.Reset();
  m_writingTransferBuffer.submitter.BeginWriting();
  return awaitable;
}

std::future<UploadResult> TransferSwapchain::UploadBuffer(BufferGPU & dstBuffer,
                                                          const uint8_t * srcData, size_t size,
                                                          size_t offset)
{
  std::promise<UploadResult> promise;
  BufferGPU stagingBuffer(GetContext(), size - offset, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  stagingBuffer.UploadSync(srcData, size, offset);

  VkBufferCopy copy{};
  copy.dstOffset = 0;
  copy.srcOffset = 0;
  copy.size = stagingBuffer.Size();
  m_writingTransferBuffer.submitter.PushCommand(vkCmdCopyBuffer, stagingBuffer.GetHandle(),
                                                dstBuffer.GetHandle(), 1, &copy);
  auto && data =
    m_writingTransferBuffer.upload_data.emplace_back(std::move(stagingBuffer), std::move(promise));
  return data.second.get_future();
}

std::future<DownloadResult> TransferSwapchain::DownloadBuffer(BufferGPU & srcBuffer, size_t size,
                                                              size_t offset)
{
  std::promise<DownloadResult> promise;
  BufferGPU stagingBuffer(GetContext(), size - offset, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  VkBufferCopy copy{};
  copy.dstOffset = 0;
  copy.srcOffset = offset;
  copy.size = size;
  m_writingTransferBuffer.submitter.PushCommand(vkCmdCopyBuffer, srcBuffer.GetHandle(),
                                                stagingBuffer.GetHandle(), 1, &copy);
  auto createDownloadResult = [](BufferGPU & stagingBuffer) -> DownloadResult
  {
    DownloadResult result(stagingBuffer.Size(), 0);
    if (auto scopedPtr = stagingBuffer.Map())
      std::memcpy(result.data(), scopedPtr.get(), stagingBuffer.Size());
    return result;
  };
  auto && data =
    m_writingTransferBuffer.download_data.emplace_back(std::move(stagingBuffer), std::move(promise),
                                                       std::move(createDownloadResult));
  return std::get<1>(data).get_future();
}

std::future<UploadResult> TransferSwapchain::UploadImage(ImageBase & dstImage,
                                                         const uint8_t * srcData,
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
  dstImage.TransferLayout(m_writingTransferBuffer.submitter, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  m_writingTransferBuffer.submitter.PushCommand(vkCmdCopyBufferToImage, stagingBuffer.GetHandle(),
                                                dstImage.GetHandle(),
                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  auto && data =
    m_writingTransferBuffer.upload_data.emplace_back(std::move(stagingBuffer), std::move(promise));
  dstImage.TransferLayout(m_writingTransferBuffer.submitter, oldLayout);
  return data.second.get_future();
}

std::future<DownloadResult> TransferSwapchain::DownloadImage(ImageBase & srcImage,
                                                             HostImageFormat format,
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
  srcImage.TransferLayout(m_writingTransferBuffer.submitter, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  m_writingTransferBuffer.submitter.PushCommand(vkCmdCopyImageToBuffer, srcImage.GetHandle(),
                                                srcImage.GetLayout(), stagingBuffer.GetHandle(), 1,
                                                &region);
  auto && data =
    m_writingTransferBuffer.download_data.emplace_back(std::move(stagingBuffer), std::move(promise),
                                                       std::move(createDownloadResult));
  srcImage.TransferLayout(m_writingTransferBuffer.submitter, oldLayout);
  return std::get<1>(data).get_future();
}

TransferSwapchain::TransferBuffer::TransferBuffer(Context & ctx, VkQueue queue,
                                                  uint32_t queueFamilyIndex)
  : submitter(ctx, queue, queueFamilyIndex, VK_PIPELINE_STAGE_TRANSFER_BIT)
{
}

} // namespace details


Transferer::Transferer(Context & ctx)
  : OwnedBy<Context>(ctx)
  , m_genericSwapchain(ctx, ctx.GetQueue(QueueType::Transfer).second,
                       ctx.GetQueue(QueueType::Transfer).first)
  , m_graphicsSwapchain(ctx, ctx.GetQueue(QueueType::Graphics).second,
                        ctx.GetQueue(QueueType::Graphics).first)
{
}

IAwaitable * Transferer::DoTransfer()
{
  std::vector<IAwaitable *> tasks{m_genericSwapchain.DoTransfer(),
                                  m_graphicsSwapchain.DoTransfer()};
  m_awaitable.SetTasks(std::move(tasks));
  return &m_awaitable;
}

std::future<UploadResult> Transferer::UploadBuffer(BufferGPU & dstBuffer, const uint8_t * srcData,
                                                   size_t size, size_t offset)
{
  return m_genericSwapchain.UploadBuffer(dstBuffer, srcData, size, offset);
}

std::future<DownloadResult> Transferer::DownloadBuffer(BufferGPU & srcBuffer, size_t size,
                                                       size_t offset)
{
  return m_genericSwapchain.DownloadBuffer(srcBuffer, size, offset);
}

std::future<UploadResult> Transferer::UploadImage(ImageBase & dstImage, const uint8_t * srcData,
                                                  const CopyImageArguments & args)
{
  return m_genericSwapchain.UploadImage(dstImage, srcData, args);
}

std::future<DownloadResult> Transferer::DownloadImage(ImageBase & srcImage, HostImageFormat format,
                                                      const ImageRegion & region)
{
  return m_graphicsSwapchain.DownloadImage(srcImage, format, region);
}

} // namespace RHI::vulkan
