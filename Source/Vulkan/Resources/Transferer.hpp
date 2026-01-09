#pragma once
#include <functional>
#include <queue>

#include <CommandsExecution/CompositeAsyncTask.hpp>
#include <CommandsExecution/Submitter.hpp>
#include <Device.hpp>
#include <ImageUtils/TextureInterface.hpp>
#include <OwnedBy.hpp>
#include <Resources/BufferGPU.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{
struct Transferer final : public OwnedBy<Context>
{
  explicit Transferer(Context & ctx);
  Transferer(Transferer && rhs);
  ~Transferer() override;

  IAwaitable * DoTransfer();

  std::future<UploadResult> UploadBuffer(VkBuffer dstBuffer, const uint8_t * srcData, size_t size,
                                         size_t offset = 0);
  std::future<DownloadResult> DownloadBuffer(VkBuffer srcBuffer, size_t size, size_t offset = 0);

  std::future<UploadResult> UploadImage(IInternalTexture & dstImage, const UploadImageArgs & args);
  std::future<DownloadResult> DownloadImage(IInternalTexture & srcImage, const DownloadImageArgs& args);
  std::future<BlitResult> BlitImageToImage(IInternalTexture & dst, IInternalTexture & src,
                                           const TextureRegion & region);

private:
  struct Bufferchain final
  {
    explicit Bufferchain(Context & ctx, QueueType type);

    details::CommandBuffer & GetWritingBuffer() & noexcept { return m_writingBuffer; }
    IAwaitable * SubmitAndSwap();

  private:
    details::Submitter m_writingBuffer;
    details::Submitter m_executingBuffer;
  };

  std::mutex m_submittingMutex;
  Bufferchain m_transferSubmitter;
  Bufferchain m_graphicsSubmitter;
  Bufferchain m_computeSubmitter;

  struct PendingTasksContainer;
  std::unique_ptr<PendingTasksContainer> m_pendingTasks;
  CompositeAsyncTask m_awaitable;
};

} // namespace RHI::vulkan
