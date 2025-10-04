#pragma once
#include <Device.hpp>
#include <FreeListPool.hpp>
#include <GarbageCollector.hpp>
#include <ImageUtils/TextureInterface.hpp>
#include <Memory/MemoryAllocator.hpp>
#include <RenderPass/Framebuffer.hpp>
#include <Resources/BufferGPU.hpp>
#include <Resources/Transferer.hpp>
#include <RHI.hpp>

namespace RHI::vulkan
{

/// @brief context is object contains vulkan logical device. Also it provides access to vulkan functions
///			If rendering system uses several GPUs, you should create one context for each physical device
struct Context final : public IContext
{
  /// @brief constructor
  explicit Context(const GpuTraits & gpuTraits, LoggingFunc log);
  RESTRICTED_COPY(Context);

public: // IContext interface
  virtual IAttachment * CreateSurfacedAttachment(const SurfaceConfig & surfaceTraits,
                                                 RenderBuffering buffering) override;
  virtual IFramebuffer * CreateFramebuffer() override;
  virtual void DestroyFramebuffer(IFramebuffer * fbo);
  virtual IBufferGPU * AllocBuffer(size_t size, BufferGPUUsage usage,
                                   bool allowHostAccess) override;
  virtual ITexture * AllocImage(const TextureDescription & args) override;
  virtual IAttachment * AllocAttachment(RHI::ImageFormat format, const RHI::TextureExtent & extent,
                                        RenderBuffering buffering,
                                        RHI::SamplesCount samplesCount) override;
  virtual void ClearResources() override; ///< GarbageCollector call
  virtual void TransferPass() override;

public: // RHI-only API
  void Log(LogMessageStatus status, const std::string & message) const noexcept;
  void WaitForIdle() const noexcept;
  bool IsValid() const noexcept { return m_validatationMark == kValidationMark; }

  const Device & GetGpuConnection() const & noexcept;
  Transferer & GetTransferer() & noexcept;
  memory::MemoryAllocator & GetBuffersAllocator() & noexcept;
  const details::VkObjectsGarbageCollector & GetGarbageCollector() const & noexcept;

  RHI::ITexture * GetNullTexture() const noexcept;

private:
  static constexpr size_t kValidationMark = 0xABCDEF00ABCDEF00;
  size_t m_validatationMark = kValidationMark;
  LoggingFunc m_logFunc;
  Device m_device;
  memory::MemoryAllocator m_allocator;
  details::VkObjectsGarbageCollector m_gc;
  std::unordered_map<std::thread::id, Transferer> m_transferers;

  // TODO: replace deque with pool
  std::deque<Framebuffer> m_framebuffers;
  std::deque<BufferGPU> m_buffers;
  std::vector<std::unique_ptr<IAttachment>> m_attachments;
  std::vector<std::unique_ptr<ITexture>> m_textures;
};

} // namespace RHI::vulkan
