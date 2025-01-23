#pragma once
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else //TODO UNIX
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "GarbageCollector.hpp"
#include "Graphics/PresentativeSwapchain.hpp"
#include "Memory/BuffersAllocator.hpp"
#include "Resources/Transferer.hpp"

namespace RHI::vulkan
{
enum class QueueType : uint8_t
{
  Present,
  Graphics,
  Compute,
  Transfer
};

/// @brief context is object contains vulkan logical device. Also it provides access to vulkan functions
///			If rendering system uses several GPUs, you should create one context for each physical device
struct Context final : public IContext
{
  /// @brief constructor
  explicit Context(const SurfaceConfig * config, LoggingFunc log);
  /// @brief destructor
  virtual ~Context() override;

public: // IContext interface
  virtual ISwapchain * GetSurfaceSwapchain() override;
  virtual std::unique_ptr<ISwapchain> CreateOffscreenSwapchain(uint32_t width, uint32_t height,
                                                               uint32_t frames_count) override;
  virtual ITransferer * GetTransferer() const noexcept override;

  virtual std::unique_ptr<IBufferGPU> AllocBuffer(size_t size, BufferGPUUsage usage,
                                                  bool mapped = false) const override;
  virtual std::unique_ptr<IImageGPU> AllocImage(const ImageDescription & args) const override;

  virtual void ClearResources() override;

public: // RHI-only API
  VkInstance GetInstance() const noexcept;
  VkDevice GetDevice() const noexcept;
  VkPhysicalDevice GetGPU() const noexcept;
  const VkPhysicalDeviceProperties & GetGpuProperties() const & noexcept;
  std::pair<uint32_t, VkQueue> GetQueue(QueueType type) const;
  uint32_t GetVulkanVersion() const noexcept;
  void Log(LogMessageStatus status, const std::string & message) const noexcept;
  void WaitForIdle() const noexcept;

  Transferer * GetInternalTransferer() const noexcept;
  const memory::BuffersAllocator & GetBuffersAllocator() const & noexcept;
  const details::VkObjectsGarbageCollector & GetGarbageCollector() const & noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
  std::unique_ptr<memory::BuffersAllocator> m_allocator;
  std::unique_ptr<details::VkObjectsGarbageCollector> m_gc;

  std::unique_ptr<Transferer> m_transferer;                  // TODO: potentially there is a list
  std::unique_ptr<PresentativeSwapchain> m_surfaceSwapchain; // TODO: potentially there is a list
  LoggingFunc m_logFunc;

private:
  Context(const Context &) = delete;
  Context & operator=(const Context &) = delete;
};

} // namespace RHI::vulkan

namespace RHI::vulkan::utils
{

/// @brief creates semaphore, doesn't own it
VkSemaphore CreateVkSemaphore(VkDevice device);
/// @brief creates fence, doesn't own it
VkFence CreateFence(VkDevice device, bool locked = false);

} // namespace RHI::vulkan::utils
