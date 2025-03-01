#pragma once
#include <functional>
#include <queue>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/CompositeAsyncTask.hpp"
#include "../CommandsExecution/Submitter.hpp"
#include "../Images/Image.hpp"
#include "BufferGPU.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

namespace details
{
struct Transferer final : public OwnedBy<Context>
{
  Transferer(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex);
  Transferer(Transferer && rhs) noexcept;
  Transferer & operator=(Transferer && rhs) noexcept;
  IAwaitable * DoTransfer();

  std::future<UploadResult> UploadBuffer(BufferGPU & dstBuffer, const uint8_t * srcData,
                                         size_t size, size_t offset = 0);
  std::future<DownloadResult> DownloadBuffer(BufferGPU & srcBuffer, size_t size, size_t offset = 0);

  std::future<UploadResult> UploadImage(Image & dstImage, const uint8_t * srcData,
                                        const CopyImageArguments & args);
  std::future<DownloadResult> DownloadImage(Image & srcImage, HostImageFormat format,
                                            const ImageRegion & region);

private:
  using CreateDownloadResultFunc = std::function<DownloadResult(BufferGPU &)>;
  using UploadData = std::pair<BufferGPU /*stagingBuffer*/, std::promise<UploadResult>>;
  using DownloadData =
    std::tuple<BufferGPU /*stagingBuffer*/, std::promise<DownloadResult>, CreateDownloadResultFunc>;

  struct TransferBuffer final
  {
    std::vector<UploadData> upload_data;
    std::vector<DownloadData> download_data;
    details::Submitter submitter;
    TransferBuffer(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex);
  };

  uint32_t m_queueFamilyIndex;
  VkQueue m_queue;
  TransferBuffer m_writingTransferBuffer;
  TransferBuffer m_executingTransferBuffer;

  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
};
} // namespace details


struct Transferer final : public OwnedBy<Context>
{
  explicit Transferer(Context & ctx);
  IAwaitable * DoTransfer();

  std::future<UploadResult> UploadBuffer(BufferGPU & dstBuffer, const uint8_t * srcData,
                                         size_t size, size_t offset = 0);
  std::future<DownloadResult> DownloadBuffer(BufferGPU & srcBuffer, size_t size, size_t offset = 0);

  std::future<UploadResult> UploadImage(Image & dstImage, const uint8_t * srcData,
                                        const CopyImageArguments & args);
  std::future<DownloadResult> DownloadImage(Image & srcImage, HostImageFormat format,
                                            const ImageRegion & region);

private:
  details::Transferer m_genericSwapchain;
  details::Transferer m_graphicsSwapchain;
  CompositeAsyncTask m_awaitable;
};

} // namespace RHI::vulkan
