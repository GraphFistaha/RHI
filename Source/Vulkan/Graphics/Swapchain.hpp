#pragma once
#include <deque>
#include <vector>

#include "SwapchainBase.hpp"

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct Swapchain final : public SwapchainBase
{
  explicit Swapchain(const Context & ctx, const vk::SurfaceKHR surface);
  virtual ~Swapchain() override;

public: // SwapchainBase interface
  virtual uint32_t AcquireImage(VkSemaphore signalSemaphore) noexcept override;
  virtual bool PresentImage(uint32_t activeImage, VkSemaphore waitRenderingSemaphore) noexcept override;

public: // IInvalidable interface
  /// @brief destroys old surface data like framebuffers, images, images_views, ets and creates new
  virtual void Invalidate() override;

public: // RHI-only API
  vk::SwapchainKHR GetHandle() const noexcept;

private:
  vk::Queue m_presentQueue = VK_NULL_HANDLE;
  uint32_t m_presentQueueIndex;

  /// presentation data
  vk::SurfaceKHR m_surface;       ///< surface
  std::unique_ptr<vkb::Swapchain> m_swapchain; ///< swapchain

private:
  /// destroy and create new swapchain
  void InvalidateSwapchain();
  void DestroyHandles() noexcept;
};

} // namespace RHI::vulkan
