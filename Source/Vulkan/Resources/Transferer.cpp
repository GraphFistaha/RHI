#include "Transferer.hpp"

#include "../CommandsExecution/Submitter.hpp"
#include "../Resources/BufferGPU.hpp"

namespace RHI::vulkan
{

Transferer::Transferer(const Context & ctx)
  : m_context(ctx)
{
  auto [familyIndex, queue] = ctx.GetQueue(QueueType::Transfer);
  m_submitter = std::make_unique<details::Submitter>(ctx, queue, familyIndex);
}

SemaphoreHandle Transferer::Flush() const
{
  m_submitter->BeginWrite();
  while (!m_tasks.empty())
  {
    auto && task = m_tasks.front();
    auto && dstBuffer = std::get<0>(task);
    auto && dstImage = std::get<1>(task);
    auto && stagingBuffer = std::get<2>(task);

    if (dstBuffer)
    {
      VkBufferCopy copy{};
      copy.dstOffset = 0;
      copy.srcOffset = 0;
      copy.size = stagingBuffer.Size();
      vkCmdCopyBuffer(m_submitter->GetCommandBuffer(), stagingBuffer.GetHandle(), dstBuffer, 1,
                      &copy);
    }
    else if (dstImage)
    {
      // TODO make upload of image
      //vkCmdCopyBufferToImage(m_submitter->GetCommandBuffer(),);
    }
  }
  m_submitter->EndWrite();
  return m_submitter->Submit({});
}
void Transferer::UploadBuffer(VkBuffer dstBuffer, BufferGPU && stagingBuffer) noexcept
{
  m_tasks.emplace(UploadTask{dstBuffer, VK_NULL_HANDLE, std::move(stagingBuffer)});
}

void Transferer::UploadImage(VkImage dstImage, BufferGPU && stagingBuffer) noexcept
{
}

} // namespace RHI::vulkan
