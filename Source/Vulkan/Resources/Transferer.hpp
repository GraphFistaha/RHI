#pragma once
#include <functional>
#include <queue>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/CompositeAsyncTask.hpp"
#include "../CommandsExecution/Submitter.hpp"
#include "../Images/TextureInterface.hpp"
#include "BufferGPU.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

namespace details
{

/// Submits transfer commands to one queue (transfer or graphic)
struct TransferSubmitter final : public OwnedBy<Context>
{
  TransferSubmitter(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex);
  TransferSubmitter(TransferSubmitter && rhs) noexcept;
  TransferSubmitter & operator=(TransferSubmitter && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  /// Submits all recorded commands and starts completion
  IAwaitable * Submit();
  /// pushes task to upload buffer from host to GPU (asynchronous)
  std::future<UploadResult> UploadBuffer(VkBuffer dstBuffer, const uint8_t * srcData, size_t size,
                                         size_t offset = 0);
  /// pushes task to download buffer from GPU to host (asynchronous)
  std::future<DownloadResult> DownloadBuffer(VkBuffer srcBuffer, size_t size, size_t offset = 0);

  /// pushes task to upload image from host to GPU (asynchronous)
  std::future<UploadResult> UploadImage(IInternalTexture & dstImage, const uint8_t * srcData,
                                        const CopyImageArguments & args);
  /// pushes task to download image from GPU to host (asynchronous)
  std::future<DownloadResult> DownloadImage(IInternalTexture & srcImage, HostImageFormat format,
                                            const ImageRegion & region);
  /// pushes task to blit image to another image
  std::future<BlitResult> BlitImageToImage(IInternalTexture & dst, IInternalTexture & src,
                                           const ImageRegion & region);

private:
  /// function to copy texels from downloaded staging buffer to host memory
  using CreateDownloadResultFunc = std::function<DownloadResult(BufferGPU &)>;
  /// queued data for uploading
  using UploadData = std::pair<BufferGPU /*stagingBuffer*/, std::promise<UploadResult>>;
  /// queued data for downloading
  using DownloadData =
    std::tuple<BufferGPU /*stagingBuffer*/, std::promise<DownloadResult>, CreateDownloadResultFunc>;
  /// queued data for blitting
  using BlitData = std::promise<BlitResult>;

  struct TransferBuffer final
  {
    std::vector<UploadData> upload_data;     ///< promises to complete upload tasks
    std::vector<DownloadData> download_data; ///< promises to complete download tasks
    std::vector<BlitData> blit_data;         ///< promises to complete blit tasks
    details::Submitter submitter;            ///< command buffer (submittable) to submit commands
    TransferBuffer(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex);
  };

  uint32_t m_queueFamilyIndex;
  VkQueue m_queue;
  TransferBuffer m_writingTransferBuffer;
  TransferBuffer m_executingTransferBuffer;
};
} // namespace details


struct Transferer final : public OwnedBy<Context>
{
  explicit Transferer(Context & ctx);
  IAwaitable * DoTransfer();

  std::future<UploadResult> UploadBuffer(VkBuffer dstBuffer, const uint8_t * srcData, size_t size,
                                         size_t offset = 0);
  std::future<DownloadResult> DownloadBuffer(VkBuffer srcBuffer, size_t size, size_t offset = 0);

  std::future<UploadResult> UploadImage(IInternalTexture & dstImage, const uint8_t * srcData,
                                        const CopyImageArguments & args);
  std::future<DownloadResult> DownloadImage(IInternalTexture & srcImage, HostImageFormat format,
                                            const ImageRegion & region);
  std::future<BlitResult> BlitImageToImage(IInternalTexture & dst, IInternalTexture & src,
                                           const ImageRegion & region);


private:
  details::TransferSubmitter m_genericSwapchain;
  details::TransferSubmitter m_graphicsSwapchain;
  CompositeAsyncTask m_awaitable;
};

} // namespace RHI::vulkan
