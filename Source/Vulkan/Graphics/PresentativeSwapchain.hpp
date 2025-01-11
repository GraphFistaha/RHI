#pragma once
#include <deque>
#include <vector>

#include "Swapchain.hpp"

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct Swapchain final : public SwapchainBase
{
  explicit Swapchain(const Context & ctx, const VkSurfaceKHR surface);
  virtual ~Swapchain() override;

public: // SwapchainBase interface
  virtual std::pair<uint32_t, VkSemaphore> AcquireImage() noexcept override;
  virtual bool PresentImage(uint32_t activeImage,
                            VkSemaphore waitRenderingSemaphore) noexcept override;

public: // IInvalidable interface
  /// @brief destroys old surface data like framebuffers, images, images_views, ets and creates new
  virtual void Invalidate() override;

public: // RHI-only API
  VkSwapchainKHR GetHandle() const noexcept;

private:
  VkQueue m_presentQueue = VK_NULL_HANDLE;
  uint32_t m_presentQueueIndex;

  /// presentation data
  VkSurfaceKHR m_surface;                      ///< surface
  std::unique_ptr<vkb::Swapchain> m_swapchain; ///< swapchain
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  std::vector<VkSemaphore> m_imageAvailabilitySemaphores;
  uint32_t m_activeSemaphore = 0;

private:
  void DestroySwapchain() noexcept;
};

} // namespace RHI::vulkan
