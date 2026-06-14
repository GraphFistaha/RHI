#pragma once
#include <functional>
#include <list>
#include <mutex>
#include <vector>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Private/OwnedBy.hpp>
#include <Memory/BufferInterface.hpp>
#include <Memory/TextureInterface.hpp>
#include <RHI.hpp>
#include <TransferPass/TransferTask.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct Transferer final : public OwnedBy<Context>
{
  explicit Transferer(Context & ctx, uint32_t queueFamily, uint32_t buffersCount);
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  void RecordCommands(details::CommandBuffer & commands);
  void OnSubmit(SubmitTask & submitTask);
  void ProcessExecutingCommands();

public:
  std::shared_ptr<IAwaitable> UploadBuffer(IInternalBuffer & dstBuffer, const uint8_t * srcData,
                                           size_t size, size_t offset = 0);
  std::shared_ptr<IAwaitable> DownloadBuffer(IInternalBuffer & srcBuffer, size_t size,
                                             size_t offset = 0);

  std::shared_ptr<IAwaitable> UploadImage(IInternalTexture & dstImage,
                                          const UploadImageArgs & args);
  std::shared_ptr<IAwaitable> DownloadImage(IInternalTexture & srcImage,
                                            const DownloadImageArgs & args);
  std::shared_ptr<IAwaitable> BlitImageToImage(IInternalTexture & dst, IInternalTexture & src,
                                               const TextureRegion & region);
  std::shared_ptr<IAwaitable> GenerateMipmaps(IInternalTexture & texture);
  std::shared_ptr<IAwaitable> GenerateMipmapsByRegions(IInternalTexture & texture,
                                                       std::span<const RHI::TextureRegion> regions);

private:
  using TasksBatch = std::vector<TrasferTaskPtr>;
  std::mutex m_writeLock;
  std::mutex m_execLock;
  TasksBatch m_writingTasks;
  std::list<TasksBatch> m_executingTasks;

private:
  void WriteNewTask(TrasferTaskPtr task);
};

} // namespace RHI::vulkan
