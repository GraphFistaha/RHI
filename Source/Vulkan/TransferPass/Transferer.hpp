#pragma once
#include <functional>
#include <queue>

#include <CommandsExecution/CompositeAsyncTask.hpp>
#include <CommandsExecution/DoubleBufferedSubmitter.hpp>
#include <Device.hpp>
#include <Private/OwnedBy.hpp>
#include <Resources/BufferGPU.hpp>
#include <Resources/BufferInterface.hpp>
#include <Resources/TextureInterface.hpp>
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
  Transferer(Transferer && rhs) noexcept;
  virtual ~Transferer() override;

  IAwaitable * DoTransfer(bool flush = false);

  std::future<UploadResult> UploadBuffer(IInternalBuffer & dstBuffer, const uint8_t * srcData,
                                         size_t size, size_t offset = 0);
  std::future<DownloadResult> DownloadBuffer(IInternalBuffer & srcBuffer, size_t size,
                                             size_t offset = 0);

  std::future<UploadResult> UploadImage(IInternalTexture & dstImage, const UploadImageArgs & args);
  std::future<DownloadResult> DownloadImage(IInternalTexture & srcImage,
                                            const DownloadImageArgs & args);
  std::future<BlitResult> BlitImageToImage(IInternalTexture & dst, IInternalTexture & src,
                                           const TextureRegion & region);
  std::future<MipmapsGenerationResult> GenerateMipmaps(IInternalTexture & texture);
  std::future<MipmapsGenerationResult> GenerateMipmapsByRegions(
    IInternalTexture & texture, const std::vector<RHI::TextureRegion> & regions);

private:
  std::mutex m_submittingMutex;
  BufferedSubmitter m_transferSubmitter;
  BufferedSubmitter m_graphicsSubmitter;
  BufferedSubmitter m_computeSubmitter;

  struct PendingTasksContainer;
  std::unique_ptr<PendingTasksContainer> m_pendingTasks;
  CompositeAsyncTask m_awaitable;
};

} // namespace RHI::vulkan
