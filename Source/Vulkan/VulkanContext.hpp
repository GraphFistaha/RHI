#pragma once
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else //TODO UNIX
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "GarbageCollector.hpp"
#include "Graphics/Framebuffer.hpp"
#include "Images/TextureInterface.hpp"
#include "Memory/MemoryAllocator.hpp"
#include "Resources/BufferGPU.hpp"
#include "Resources/Transferer.hpp"

namespace RHI::vulkan
{
enum class QueueType : uint8_t
{
  Present,
  Graphics,
  Compute,
  Transfer,
  Total
};

/// @brief context is object contains vulkan logical device. Also it provides access to vulkan functions
///			If rendering system uses several GPUs, you should create one context for each physical device
struct Context final : public IContext
{
  /// @brief constructor
  explicit Context(const SurfaceConfig * config, LoggingFunc log);
  /// @brief destructor
  virtual ~Context() override;
  RESTRICTED_COPY(Context);

public: // IContext interface
  virtual IAttachment * GetSurfaceImage() override;
  virtual IFramebuffer * CreateFramebuffer(uint32_t frames_count) override;
  virtual IBufferGPU * AllocBuffer(size_t size, BufferGPUUsage usage,
                                   bool allowHostAccess) override;
  virtual ITexture * AllocImage(const ImageCreateArguments & args) override;
  virtual IAttachment * AllocAttachment(const ImageCreateArguments & args) override;
  virtual void ClearResources() override;
  virtual void Flush() override;

public: // RHI-only API
  VkInstance GetInstance() const noexcept;
  VkDevice GetDevice() const noexcept;
  VkPhysicalDevice GetGPU() const noexcept;
  const VkPhysicalDeviceProperties & GetGpuProperties() const & noexcept;
  std::pair<uint32_t, VkQueue> GetQueue(QueueType type) const;
  uint32_t GetVulkanVersion() const noexcept;
  void Log(LogMessageStatus status, const std::string & message) const noexcept;
  void WaitForIdle() const noexcept;
  bool IsValid() const noexcept { return m_validatationMark == kValidationMark; }

  Transferer & GetTransferer() & noexcept;
  const memory::MemoryAllocator & GetBuffersAllocator() const & noexcept;
  const details::VkObjectsGarbageCollector & GetGarbageCollector() const & noexcept;

  RHI::ITexture * GetNullTexture() const noexcept;

private:
  static constexpr size_t kValidationMark = 0xABCDEF00ABCDEF00;
  size_t m_validatationMark = kValidationMark;
  struct Impl;
  std::unique_ptr<Impl> m_impl;
  std::unique_ptr<memory::MemoryAllocator> m_allocator;
  std::unique_ptr<details::VkObjectsGarbageCollector> m_gc;

  std::unordered_map<std::thread::id, Transferer> m_transferers;
  // TODO: replace deque with pool
  std::deque<Framebuffer> m_framebuffers;
  std::deque<BufferGPU> m_buffers;
  std::vector<std::unique_ptr<IAttachment>> m_attachments;
  std::vector<std::unique_ptr<ITexture>> m_textures;
  LoggingFunc m_logFunc;
};

} // namespace RHI::vulkan

namespace RHI::vulkan::utils
{

/// @brief creates semaphore, doesn't own it
VkSemaphore CreateVkSemaphore(VkDevice device);
/// @brief creates fence, doesn't own it
VkFence CreateFence(VkDevice device, bool locked = false);

} // namespace RHI::vulkan::utils
