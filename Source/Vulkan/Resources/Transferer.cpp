#include "Transferer.hpp"

#include <ImageUtils/ImageFormatsConversation.hpp>
#include <ImageUtils/InternalImageTraits.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

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
                                        IInternalTexture & dstImage, const UploadImageArgs & args);
  /// pushes task to download image from GPU to host (asynchronous)
  std::future<DownloadResult> DownloadImage(details::CommandBuffer & commands,
                                            IInternalTexture & srcImage,
                                            const DownloadImageArgs & args);
  /// pushes task to blit image to another image
  std::future<BlitResult> BlitImageToImage(details::CommandBuffer & commands,
                                           IInternalTexture & dst, IInternalTexture & src,
                                           const TextureRegion & region);

  std::future<MipmapsGenerationResult> GenerateMipmaps(details::CommandBuffer & commands,
                                                       IInternalTexture & dst);

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
  /// @brief queued data for mipmaps generation
  using MipsGenerationTask = std::pair<IInternalTexture *, std::promise<MipmapsGenerationResult>>;

  struct PendingTasksBatch final
  {
    std::vector<UploadTask> upload_tasks;     ///< promises to complete upload tasks
    std::vector<DownloadTask> download_tasks; ///< promises to complete download tasks
    std::vector<BlitTask> blit_tasks;         ///< promises to complete blit tasks
    /// promises to complete mips generation tasks
    std::vector<MipsGenerationTask> mips_generation_tasks;
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

  for (auto && [texture, promise] : m_executingBatch.mips_generation_tasks)
  {
    promise.set_value(texture->GetMipLevelsCount());
  }
  m_executingBatch.mips_generation_tasks.clear();

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
  copy.size = size - offset;
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
  details::CommandBuffer & commands, IInternalTexture & dstImage, const UploadImageArgs & args)
{
  std::promise<UploadResult> promise;
  const size_t copyingRegionSize =
    RHI::utils::GetSizeOfImage(args.copyRegion.extent, dstImage.GetInternalFormat());
  BufferGPU stagingBuffer(GetContext(), copyingRegionSize, g_stagingUsage, true);
  if (auto && mapped_ptr = stagingBuffer.Map())
  {
    MappedGpuTextureView gpuTexture{};
    gpuTexture.pixelData = reinterpret_cast<uint8_t *>(mapped_ptr.get());
    gpuTexture.extent = args.copyRegion.extent;
    gpuTexture.format = dstImage.GetInternalFormat();
    gpuTexture.baseLayerIndex = args.layerIndex;
    gpuTexture.layersCount = args.layersCount;
    auto dstExtent = dstImage.GetInternalExtent();
    CopyImageFromHost(args.srcTexture, gpuTexture, args.copyRegion);
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
    region.imageExtent = {args.copyRegion.extent[0], args.copyRegion.extent[1],
                          args.copyRegion.extent[2]};
    region.imageOffset = {static_cast<int>(args.copyRegion.offset[0]),
                          static_cast<int>(args.copyRegion.offset[1]),
                          static_cast<int>(args.copyRegion.offset[2])};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = args.layerIndex;
    region.imageSubresource.layerCount = args.layersCount;
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
  details::CommandBuffer & commands, IInternalTexture & srcImage, const DownloadImageArgs & args)
{
  std::promise<DownloadResult> promise;
  BufferGPU stagingBuffer(GetContext(),
                          RHI::utils::GetSizeOfImage(args.copyRegion.extent,
                                                     srcImage.GetInternalFormat()),
                          g_stagingUsage, true);
  VkBufferImageCopy region{};
  {
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageExtent = {args.copyRegion.extent[0], args.copyRegion.extent[1],
                          args.copyRegion.extent[2]};
    region.imageOffset = {static_cast<int>(args.copyRegion.offset[0]),
                          static_cast<int>(args.copyRegion.offset[1]),
                          static_cast<int>(args.copyRegion.offset[2])};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = args.layerIndex;
    region.imageSubresource.layerCount = args.layersCount;
  }

  auto createDownloadResult =
    [args, srcFormat = srcImage.GetInternalFormat()](BufferGPU & stagingBuffer) -> DownloadResult
  {
    DownloadResult result(RHI::utils::GetSizeOfImage(args.copyRegion.extent, args.format));
    HostTextureView hostTexture{};
    hostTexture.extent = args.copyRegion.extent;
    hostTexture.format = args.format;
    hostTexture.pixelData = result.data();
    if (auto scopedPtr = stagingBuffer.Map())
    {
      MappedGpuTextureView view{};
      view.pixelData = reinterpret_cast<uint8_t *>(scopedPtr.get());
      view.extent = args.copyRegion.extent;
      view.format = srcFormat;
      CopyImageToHost(view, hostTexture, {{0, 0, 0}, args.copyRegion.extent});
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
  const TextureRegion & region)
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

std::future<MipmapsGenerationResult> Transferer::PendingTasksContainer::GenerateMipmaps(
  details::CommandBuffer & commands, IInternalTexture & dst)
{
  // derives extent in 2
  auto extentDiv2 = [](const VkOffset3D & extent)
  {
    return VkOffset3D{std::max(1, extent.x / 2), std::max(1, extent.y / 2),
                      std::max(1, extent.z / 2)};
  };


  const uint32_t transferQueue =
    GetContext().GetGpuConnection().GetQueue(RHI::vulkan::QueueType::Transfer).first;

  // lambda to make a barrier for mip level
  auto transferLayoutForMipLevel = [&commands, &dst, transferQueue](VkImageLayout oldLayout,
                                                                    VkImageLayout newLayout,
                                                                    uint32_t level)
  {
    assert(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
           oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    assert(newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
           newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageMemoryBarrier barrier{};
    {
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = oldLayout;
      barrier.newLayout = newLayout;
      barrier.srcQueueFamilyIndex = transferQueue;
      barrier.dstQueueFamilyIndex = transferQueue;
      barrier.image = dst.GetHandle();
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.baseMipLevel = level;
      barrier.subresourceRange.levelCount = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
      barrier.srcAccessMask = newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                              ? VK_ACCESS_TRANSFER_WRITE_BIT
                              : VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                              ? VK_ACCESS_TRANSFER_READ_BIT
                              : VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    commands.PushCommand(vkCmdPipelineBarrier, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  };

  // if texture has no mip levels, then do nothing
  if (dst.GetMipLevelsCount() <= 1)
  {
    std::promise<MipmapsGenerationResult> result;
    result.set_value(0);
    return result.get_future();
  }

  // help variables for algorithm
  VkExtent3D extent = dst.GetInternalExtent();
  VkOffset3D oldMipExtent = {static_cast<int>(extent.width), static_cast<int>(extent.height),
                             static_cast<int>(extent.depth)};
  VkOffset3D mipExtent = extentDiv2(oldMipExtent);

  /*
    Algorithm description:
    Given an texture with N layers and M mip levels to generate.
    you should generate all mip levels for each layer
    note: the texture must be in the same layout as it was before the algorithm

    1) remember layout of the texture to restore it after the execution
    2) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for all layers/mipLevels
    3) for i = 1 to M:
         3.1) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL for i - 1 mip level. 
                This step blocks mip level for reading
         3.2) blit image (all N layers) from i - 1 to i mip level with linear filteration. 
                Note: i'th level has only half of i-1'th level's extent
         3.3) transfer layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for i - 1 mip level
                this step waits for reading is completed and blocks for writing
         3.4) div extent in 2
    4) restore old layout. After the loop, the texture is in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout
  */

  VkImageLayout oldLayout = dst.GetLayout();
  dst.TransferLayout(commands, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  for (uint32_t level = 1; level < dst.GetMipLevelsCount(); ++level)
  {
    transferLayoutForMipLevel(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, level - 1);

    VkImageBlit blit{};
    {
      blit.srcOffsets[0] = {0, 0, 0};
      blit.srcOffsets[1] = oldMipExtent;
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.mipLevel = level - 1;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.srcSubresource.layerCount = dst.GetLayersCount();
      blit.dstOffsets[0] = {0, 0, 0};
      blit.dstOffsets[1] = mipExtent;
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.mipLevel = level;
      blit.dstSubresource.baseArrayLayer = 0;
      blit.dstSubresource.layerCount = dst.GetLayersCount();
    }

    commands.PushCommand(vkCmdBlitImage, dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                         VK_FILTER_LINEAR);

    transferLayoutForMipLevel(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, level - 1);

    oldMipExtent = mipExtent;
    mipExtent = extentDiv2(mipExtent);
  }

  dst.TransferLayout(commands, oldLayout);

  std::promise<MipmapsGenerationResult> promise;
  auto && data = m_writingBatch.mips_generation_tasks.emplace_back(&dst, std::move(promise));
  return data.second.get_future();
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
                                                  const UploadImageArgs & args)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->UploadImage(m_transferSubmitter.GetWritingBuffer(), dstImage, args);
}

std::future<DownloadResult> Transferer::DownloadImage(IInternalTexture & srcImage,
                                                      const DownloadImageArgs & args)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->DownloadImage(m_graphicsSubmitter.GetWritingBuffer(), srcImage, args);
}

std::future<BlitResult> Transferer::BlitImageToImage(IInternalTexture & dst, IInternalTexture & src,
                                                     const TextureRegion & region)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->BlitImageToImage(m_graphicsSubmitter.GetWritingBuffer(), dst, src, region);
}

std::future<MipmapsGenerationResult> Transferer::GenerateMipmaps(IInternalTexture & texture)
{
  std::lock_guard lk{m_submittingMutex};
  return m_pendingTasks->GenerateMipmaps(m_graphicsSubmitter.GetWritingBuffer(), texture);
}

Transferer::Bufferchain::Bufferchain(Context & ctx, QueueType type)
  : m_writingBuffer(ctx, type, VK_PIPELINE_STAGE_TRANSFER_BIT)
  , m_executingBuffer(ctx, type, VK_PIPELINE_STAGE_TRANSFER_BIT)
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
