#pragma once
#include <queue>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/Submitter.hpp"
#include "../ContextualObject.hpp"
#include "../Images/ImageBase.hpp"
#include "BufferGPU.hpp"

namespace RHI::vulkan
{

struct Transferer final : public ContextualObject
{
  using ByteBuffer = std::vector<uint8_t>;

  explicit Transferer(Context & ctx);
  Transferer(Transferer && rhs) noexcept;
  Transferer & operator=(Transferer && rhs) noexcept;

public:
  IAwaitable * DoTransfer();

public:
  void UploadBuffer(BufferGPU * dstBuffer, const uint8_t * srcData, size_t size,
                    size_t offset = 0) noexcept;
  std::future<ByteBuffer> DownloadBuffer(BufferGPU * srcBuffer, size_t size,
                                         size_t offset = 0) noexcept;

  void UploadImage(ImageBase * dstImage, BufferGPU && stagingBuffer) noexcept;
  std::future<ByteBuffer> DownloadImage(ImageBase * srcImage,
                                        const CopyImageArguments & args) noexcept;

private:
  using UploadData = BufferGPU;
  using DownloadData = std::pair<BufferGPU, std::promise<ByteBuffer>>;

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
