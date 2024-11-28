#pragma once
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else //TODO UNIX
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

namespace RHI::vulkan
{
constexpr const char * apiFolder = ".vulkan";
constexpr const char * shaderExtension = ".spv";

enum class QueueType : uint8_t
{
  Present,
  Graphics,
  Compute,
  Transfer
};

namespace details
{
struct BuffersAllocator;
}
struct Swapchain;
struct Transferer;

struct BufferGPU;
struct ImageGPU;

/// @brief context is object contains vulkan logical device. Also it provides access to vulkan functions
///			If rendering system uses several GPUs, you should create one context for each physical device
struct Context final : public IContext
{
  /// @brief constructor
  explicit Context(const SurfaceConfig & config, LoggingFunc log);
  /// @brief destructor
  virtual ~Context() override;

public: // IContext interface
  virtual ISwapchain * GetSurfaceSwapchain() override;
  virtual ITransferer * GetTransferer() override;

  virtual std::unique_ptr<IBufferGPU> AllocBuffer(size_t size, BufferGPUUsage usage,
                                                  bool mapped = false) const override;
  virtual std::unique_ptr<IImageGPU> AllocImage(const ImageCreateArguments & args) const override;

  virtual void WaitForIdle() const noexcept override;
public: // RHI-only API
  const vk::Instance GetInstance() const;
  const vk::Device GetDevice() const;
  const vk::PhysicalDevice GetGPU() const;
  std::pair<uint32_t, VkQueue> GetQueue(QueueType type) const;
  uint32_t GetVulkanVersion() const;
  void Log(LogMessageStatus status, const std::string & message) const noexcept;

  const details::BuffersAllocator & GetBuffersAllocator() const & noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
  std::unique_ptr<details::BuffersAllocator> m_allocator;
  std::unique_ptr<Swapchain> m_surfaceSwapchain;
  std::unique_ptr<Transferer> m_transferer;
  LoggingFunc m_logFunc;

private:
  Context(const Context &) = delete;
  Context & operator=(const Context &) = delete;
};

} // namespace RHI::vulkan

namespace RHI::vulkan::utils
{

/// @brief creates semaphore, doesn't own it
vk::Semaphore CreateVkSemaphore(vk::Device device);
/// @brief creates fence, doesn't own it
vk::Fence CreateFence(vk::Device device, bool locked = false);

template<typename InternalClassT, typename InterfaceClassT>
decltype(auto) CastInterfaceClass2Internal(InterfaceClassT && obj)
{
  return dynamic_cast<const InternalClassT &>(std::forward<InterfaceClassT>(obj));
}

template<typename VulkanEnumT, typename InterfaceEnumT>
VulkanEnumT CastInterfaceEnum2Vulkan(InterfaceEnumT value);

} // namespace RHI::vulkan::utils
