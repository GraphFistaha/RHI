#pragma once
#include <queue>

#include "../VulkanContext.hpp"

namespace RHI::vulkan
{
namespace details
{
struct Submitter;
}
struct BufferGPU;

struct Transferer : public ITransferer
{
  explicit Transferer(const Context & ctx);

public:
  virtual SemaphoreHandle Flush() const override;

public:
  void UploadBuffer(VkBuffer dstBuffer, BufferGPU && stagingBuffer) noexcept;
  void UploadImage(VkImage dstImage, BufferGPU && stagingBuffer) noexcept;
private:
  using UploadTask = std::tuple<VkBuffer, VkImage, BufferGPU>;

  const Context & m_context;
  std::unique_ptr<details::Submitter> m_submitter;
  std::queue<UploadTask> m_tasks;
};

} // namespace RHI::vulkan
