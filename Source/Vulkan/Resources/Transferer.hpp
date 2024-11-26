#pragma once
#include <queue>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>
#include "../CommandsExecution/Submitter.hpp"
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
  virtual SemaphoreHandle Flush() override;

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
