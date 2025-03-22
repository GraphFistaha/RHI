#pragma once
#include <functional>
#include <queue>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/CompositeAsyncTask.hpp"
#include "../CommandsExecution/Submitter.hpp"
#include "../Graphics/TextureInterface.hpp"
#include "BufferGPU.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

namespace details
{
struct TransferSubmitter final : public OwnedBy<Context>
{
  TransferSubmitter(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex);
  TransferSubmitter(TransferSubmitter && rhs) noexcept;
  TransferSubmitter & operator=(TransferSubmitter && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  IAwaitable * Submit();

  std::future<UploadResult> UploadBuffer(VkBuffer dstBuffer, const uint8_t * srcData, size_t size,
                                         size_t offset = 0);
  std::future<DownloadResult> DownloadBuffer(VkBuffer srcBuffer, size_t size, size_t offset = 0);

  std::future<UploadResult> UploadImage(ITexture & dstImage, const uint8_t * srcData,
                                        const CopyImageArguments & args);
  std::future<DownloadResult> DownloadImage(ITexture & srcImage, HostImageFormat format,
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
};
} // namespace details


struct Transferer final : public OwnedBy<Context>
{
  explicit Transferer(Context & ctx);
  IAwaitable * DoTransfer();

  std::future<UploadResult> UploadBuffer(VkBuffer dstBuffer, const uint8_t * srcData, size_t size,
                                         size_t offset = 0);
  std::future<DownloadResult> DownloadBuffer(VkBuffer srcBuffer, size_t size, size_t offset = 0);

  std::future<UploadResult> UploadImage(ITexture & dstImage, const uint8_t * srcData,
                                        const CopyImageArguments & args);
  std::future<DownloadResult> DownloadImage(ITexture & srcImage, HostImageFormat format,
                                            const ImageRegion & region);

private:
  details::TransferSubmitter m_genericSwapchain;
  details::TransferSubmitter m_graphicsSwapchain;
  CompositeAsyncTask m_awaitable;
};

} // namespace RHI::vulkan
