#include "Transferer.hpp"

#include "../Images/ImageFormatsConversation.hpp"
#include "../Images/InternalImageTraits.hpp"
#include "../Utils/CastHelper.hpp"
#include "../VulkanContext.hpp"

namespace RHI::vulkan
{

static constexpr uint32_t g_stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT;

/// Submits transfer commands to one queue (transfer or graphic)
struct Transferer::PendingTasksContainer final : public OwnedBy<Context>
{
  PendingTasksContainer(Context & ctx);
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  void ProcessSubmittedTasks();

public:
  /// pushes task to upload buffer from host to GPU (asynchronous)
  std::future<UploadResult> UploadBuffer(details::CommandBuffer & commands, VkBuffer dstBuffer,
                                         const uint8_t * srcData, size_t size, size_t offset = 0);
  /// pushes task to download buffer from GPU to host (asynchronous)
  std::future<DownloadResult> DownloadBuffer(details::CommandBuffer & commands, VkBuffer srcBuffer,
                                             size_t size, size_t offset = 0);

  /// pushes task to upload image from host to GPU (asynchronous)
  std::future<UploadResult> UploadImage(details::CommandBuffer & commands,
                                        IInternalTexture & dstImage, const uint8_t * srcData,
                                        const CopyImageArguments & args);
  /// pushes task to download image from GPU to host (asynchronous)
  std::future<DownloadResult> DownloadImage(details::CommandBuffer & commands,
                                            IInternalTexture & srcImage, HostImageFormat format,
                                            const ImageRegion & region);
  /// pushes task to blit image to another image
  std::future<BlitResult> BlitImageToImage(details::CommandBuffer & commands,
                                           IInternalTexture & dst, IInternalTexture & src,
                                           const ImageRegion & region);

private:
  /// function to copy texels from downloaded staging buffer to host memory
  using CreateDownloadResultFunc = std::function<DownloadResult(BufferGPU &)>;
  /// queued data for uploading
  using UploadTask = std::pair<BufferGPU /*stagingBuffer*/, std::promise<UploadResult>>;
  /// queued data for downloading
  using DownloadTask =
    std::tuple<BufferGPU /*stagingBuffer*/, std::promise<DownloadResult>, CreateDownloadResultFunc>;
  /// queued data for blitting
  using BlitTask = std::promise<BlitResult>;

  struct PendingTasksBatch final
  {
    std::vector<UploadTask> upload_tasks;     ///< promises to complete upload tasks
    std::vector<DownloadTask> download_tasks; ///< promises to complete download tasks
    std::vector<BlitTask> blit_tasks;         ///< promises to complete blit tasks
  };

  PendingTasksBatch m_writingBatch;
  PendingTasksBatch m_executingBatch;
};


Transferer::PendingTasksContainer::PendingTasksContainer(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

void Transferer::PendingTasksContainer::ProcessSubmittedTasks()
{
  // process upload data
  for (auto && [stagingBuffer, promise] : m_executingBatch.upload_tasks)
  {
    UploadResult result = stagingBuffer.Size();
    promise.set_value(result);
  }
  m_executingBatch.upload_tasks.clear();

  // process download data
  for (auto && [stagingBuffer, promise, createDownloadResult] : m_executingBatch.download_tasks)
  {
    DownloadResult result = createDownloadResult(stagingBuffer);
    promise.set_value(std::move(result));
  }
  m_executingBatch.download_tasks.clear();

  // process blitting commands
  for (auto && promise : m_executingBatch.blit_tasks)
  {
    BlitResult result = 0;
    promise.set_value(result);
  }
  m_executingBatch.blit_tasks.clear();

  std::swap(m_executingBatch, m_writingBatch);
}

std::future<UploadResult> Transferer::PendingTasksContainer::UploadBuffer(
  details::CommandBuffer & commands, VkBuffer dstBuffer, const uint8_t * srcData, size_t size,
  size_t offset)
{
  std::promise<UploadResult> promise;
  BufferGPU stagingBuffer(GetContext(), size - offset, g_stagingUsage, true);
  stagingBuffer.UploadSync(srcData, size, offset);

  VkBufferCopy copy{};
  copy.dstOffset = 0;
  copy.srcOffset = 0;
  copy.size = stagingBuffer.Size();
  commands.PushCommand(vkCmdCopyBuffer, stagingBuffer.GetHandle(), dstBuffer, 1, &copy);
  auto && data =
    m_writingBatch.upload_tasks.emplace_back(std::move(stagingBuffer), std::move(promise));
  return data.second.get_future();
}

std::future<DownloadResult> Transferer::PendingTasksContainer::DownloadBuffer(
  details::CommandBuffer & commands, VkBuffer srcBuffer, size_t size, size_t offset)
{
  std::promise<DownloadResult> promise;
  BufferGPU stagingBuffer(GetContext(), size - offset, g_stagingUsage, true);
  VkBufferCopy copy{};
  copy.dstOffset = 0;
  copy.srcOffset = offset;
  copy.size = size;
  commands.PushCommand(vkCmdCopyBuffer, srcBuffer, stagingBuffer.GetHandle(), 1, &copy);
  auto createDownloadResult = [](BufferGPU & stagingBuffer) -> DownloadResult
  {
    DownloadResult result(stagingBuffer.Size(), 0);
    if (auto scopedPtr = stagingBuffer.Map())
      std::memcpy(result.data(), scopedPtr.get(), stagingBuffer.Size());
    return result;
  };
  auto && data = m_writingBatch.download_tasks.emplace_back(std::move(stagingBuffer),
                                                            std::move(promise),
                                                            std::move(createDownloadResult));
  return std::get<1>(data).get_future();
}

std::future<UploadResult> Transferer::PendingTasksContainer::UploadImage(
  details::CommandBuffer & commands, IInternalTexture & dstImage, const uint8_t * srcData,
  const CopyImageArguments & args)
{
  std::promise<UploadResult> promise;
  const size_t copyingRegionSize =
    RHI::utils::GetSizeOfImage(args.src.extent, dstImage.GetInternalFormat());
  BufferGPU stagingBuffer(GetContext(), copyingRegionSize, g_stagingUsage, true);
  if (auto && mapped_ptr = stagingBuffer.Map())
  {
    CopyImageFromHost(srcData, args.src.extent, args.src, args.hostFormat,
                      reinterpret_cast<uint8_t *>(mapped_ptr.get()), args.dst.extent, args.dst,
                      dstImage.GetInternalFormat());
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
  dstImage.TransferLayout(commands, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  commands.PushCommand(vkCmdCopyBufferToImage, stagingBuffer.GetHandle(), dstImage.GetHandle(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  auto && data =
    m_writingBatch.upload_tasks.emplace_back(std::move(stagingBuffer), std::move(promise));
  dstImage.TransferLayout(commands, oldLayout);
  return data.second.get_future();
}

std::future<DownloadResult> Transferer::PendingTasksContainer::DownloadImage(
  details::CommandBuffer & commands, IInternalTexture & srcImage, HostImageFormat format,
  const ImageRegion & imgRegion)
{
  std::promise<DownloadResult> promise;
  BufferGPU stagingBuffer(GetContext(),
                          RHI::utils::GetSizeOfImage(imgRegion.extent,
                                                     srcImage.GetInternalFormat()),
                          g_stagingUsage, true);
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

  auto createDownloadResult = [imgRegion, format, srcFormat = srcImage.GetInternalFormat()](
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
  srcImage.TransferLayout(commands, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  commands.PushCommand(vkCmdCopyImageToBuffer, srcImage.GetHandle(), srcImage.GetLayout(),
                       stagingBuffer.GetHandle(), 1, &region);
  auto && data = m_writingBatch.download_tasks.emplace_back(std::move(stagingBuffer),
                                                            std::move(promise),
                                                            std::move(createDownloadResult));
  srcImage.TransferLayout(commands, oldLayout);
  return std::get<1>(data).get_future();
}

std::future<BlitResult> Transferer::PendingTasksContainer::BlitImageToImage(
  details::CommandBuffer & commands, IInternalTexture & dst, IInternalTexture & src,
  const ImageRegion & region)
{
  std::promise<BlitResult> promise;
  VkImageCopy copy{};
  {
    //TODO: Fill copy
    copy.extent = src.GetInternalExtent();
    //copy.dstOffset =
  }
  commands.PushCommand(vkCmdCopyImage, src.GetHandle(), src.GetLayout(), dst.GetHandle(),
                       dst.GetLayout(), 1, &copy);
  auto && data = m_writingBatch.blit_tasks.emplace_back(std::move(promise));
  return data.get_future();
}


Transferer::Transferer(Context & ctx)
  : OwnedBy<Context>(ctx)
  , m_transferSubmitter(ctx, QueueType::Transfer)
  , m_graphicsSubmitter(ctx, QueueType::Graphics)
  , m_computeSubmitter(ctx, QueueType::Compute)
  , m_pendingTasks(new Transferer::PendingTasksContainer(ctx))
{
}


Transferer::Transferer(Transferer && rhs)
  : Transferer(rhs.GetOwner())
{
  std::swap(m_transferSubmitter, rhs.m_transferSubmitter);
  std::swap(m_graphicsSubmitter, rhs.m_graphicsSubmitter);
  std::swap(m_computeSubmitter, rhs.m_computeSubmitter);
  std::swap(m_awaitable, rhs.m_awaitable);
  std::swap(m_pendingTasks, rhs.m_pendingTasks);
}

Transferer::~Transferer() = default;

IAwaitable * Transferer::DoTransfer()
{
  std::lock_guard lk{m_submittingMutex};
  std::vector<IAwaitable *> tasks{m_transferSubmitter.SubmitAndSwap(),
                                  m_graphicsSubmitter.SubmitAndSwap(),
                                  m_computeSubmitter.SubmitAndSwap()};
  m_awaitable.SetTasks(std::move(tasks));
  m_pendingTasks->ProcessSubmittedTasks();
  return &m_awaitable;
}

std::future<UploadResult> Transferer::UploadBuffer(VkBuffer dstBuffer, const uint8_t * srcData,
                                                   size_t size, size_t offset)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->UploadBuffer(m_transferSubmitter.GetWritingBuffer(), dstBuffer, srcData,
                                      size, offset);
}

std::future<DownloadResult> Transferer::DownloadBuffer(VkBuffer srcBuffer, size_t size,
                                                       size_t offset)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->DownloadBuffer(m_transferSubmitter.GetWritingBuffer(), srcBuffer, size,
                                        offset);
}

std::future<UploadResult> Transferer::UploadImage(IInternalTexture & dstImage,
                                                  const uint8_t * srcData,
                                                  const CopyImageArguments & args)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->UploadImage(m_transferSubmitter.GetWritingBuffer(), dstImage, srcData,
                                     args);
}

std::future<DownloadResult> Transferer::DownloadImage(IInternalTexture & srcImage,
                                                      HostImageFormat format,
                                                      const ImageRegion & region)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->DownloadImage(m_graphicsSubmitter.GetWritingBuffer(), srcImage, format,
                                       region);
}

std::future<BlitResult> Transferer::BlitImageToImage(IInternalTexture & dst, IInternalTexture & src,
                                                     const ImageRegion & region)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->BlitImageToImage(m_graphicsSubmitter.GetWritingBuffer(), dst, src, region);
}

Transferer::Bufferchain::Bufferchain(Context & ctx, QueueType type)
  : m_writingBuffer(ctx, ctx.GetQueue(type).second, ctx.GetQueue(type).first,
                    VK_PIPELINE_STAGE_TRANSFER_BIT)
  , m_executingBuffer(ctx, ctx.GetQueue(type).second, ctx.GetQueue(type).first,
                      VK_PIPELINE_STAGE_TRANSFER_BIT)
{
  m_writingBuffer.BeginWriting();
}

IAwaitable * Transferer::Bufferchain::SubmitAndSwap()
{
  if (m_writingBuffer.IsEmpty())
    return nullptr;
  m_writingBuffer.EndWriting();
  IAwaitable * result = m_writingBuffer.Submit(true, {});
  std::swap(m_writingBuffer, m_executingBuffer);
  m_writingBuffer.WaitForSubmitCompleted();
  m_writingBuffer.Reset();
  m_writingBuffer.BeginWriting();
  return result;
}

} // namespace RHI::vulkan
