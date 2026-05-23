#pragma once
#include <functional>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Device.hpp>
#include <Private/OwnedBy.hpp>
#include <Resources/BufferInterface.hpp>
#include <Resources/TextureInterface.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
struct Context;
} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct Transferer final : public OwnedBy<Context>
{
  explicit Transferer(Context & ctx, uint32_t queueFamily, uint32_t buffersCount);
  virtual ~Transferer() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  void FlushCommands(details::CommandBuffer & commands);

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
  const uint32_t m_queueFamily;
  std::mutex m_writeLock;
  details::CommandBuffer m_writeBuffer;
  details::CommandBuffer m_execBuffer;

  struct PendingTasksContainer;
  std::unique_ptr<PendingTasksContainer> m_pendingTasks;

private:
  details::CommandBuffer & GetWritingBuffer() & noexcept;
};

} // namespace RHI::vulkan
