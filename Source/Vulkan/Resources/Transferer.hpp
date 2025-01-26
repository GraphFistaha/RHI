#pragma once
#include <queue>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../CommandsExecution/Submitter.hpp"
#include "../Images/ImageBase.hpp"
#include "BufferGPU.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct Transferer : public ITransferer
{
  explicit Transferer(const Context & ctx);

public: // ITransferer interface
  virtual IAwaitable * DoTransfer() override;

public:
  void UploadBuffer(BufferGPU * dstBuffer, BufferGPU && stagingBuffer) noexcept;
  void UploadImage(ImageBase * dstImage, BufferGPU && stagingBuffer) noexcept;
  
private:
  using UploadTask = std::tuple<BufferGPU *, ImageBase *, BufferGPU>;
  using DownloadTask = std::tuple<>;

  const Context & m_context;
  uint32_t m_queueFamilyIndex;
  VkQueue m_queue;
  details::Submitter m_submitter;
  std::queue<UploadTask> m_tasks;
};

} // namespace RHI::vulkan
