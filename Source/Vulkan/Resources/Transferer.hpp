#pragma once
#include <functional>
#include <queue>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/Submitter.hpp"
#include "../Images/ImageBase.hpp"
#include "BufferGPU.hpp"

namespace RHI::vulkan
{

struct Transferer final : public OwnedBy<Context>
{
  explicit Transferer(Context & ctx);
  Transferer(Transferer && rhs) noexcept;
  Transferer & operator=(Transferer && rhs) noexcept;

public:
  IAwaitable * DoTransfer();

public:
  std::future<UploadResult> UploadBuffer(BufferGPU & dstBuffer, const uint8_t * srcData,
                                         size_t size, size_t offset = 0);
  std::future<DownloadResult> DownloadBuffer(BufferGPU & srcBuffer, size_t size, size_t offset = 0);

  std::future<UploadResult> UploadImage(ImageBase & dstImage, const uint8_t * srcData,
                                        const CopyImageArguments & args);
  std::future<DownloadResult> DownloadImage(ImageBase & srcImage, HostImageFormat format,
                                            const ImageRegion & region);

  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

private:
  using CreateDownloadResultFunc = std::function<DownloadResult(BufferGPU &)>;
  using UploadData = std::pair<BufferGPU /*stagingBuffer*/, std::promise<UploadResult>>;
  using DownloadData =
    std::tuple<BufferGPU /*stagingBuffer*/, std::promise<DownloadResult>, CreateDownloadResultFunc>;

  struct TransferData
  {
    details::Submitter submitter;
    std::vector<UploadData> upload_data;
    std::vector<DownloadData> download_data;
    TransferData(Context & ctx, VkQueue queue, uint32_t queueFamilyIndex);
  };

  uint32_t m_queueFamilyIndex;
  VkQueue m_queue;
  TransferData m_writingTransferData;
  TransferData m_executingTransferData;
};

} // namespace RHI::vulkan
