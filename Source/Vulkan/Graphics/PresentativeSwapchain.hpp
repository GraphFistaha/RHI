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
struct PresentativeSwapchain final : public Swapchain
{
  explicit PresentativeSwapchain(const Context & ctx, const VkSurfaceKHR surface);
  virtual ~PresentativeSwapchain() override;

public: // RHI-only API
  VkSwapchainKHR GetHandle() const noexcept;

  virtual std::pair<uint32_t, VkSemaphore> AcquireImage() override;
  virtual bool FinishImage(uint32_t activeImage, VkSemaphore waitRenderingSemaphore) override;
  /// @brief destroys old surface data like framebuffers, images, images_views, ets and creates new
  virtual void Invalidate() override;

protected:
  virtual void InvalidateAttachments() override;

private:
  VkQueue m_presentQueue = VK_NULL_HANDLE;
  uint32_t m_presentQueueIndex;

  /// presentation data
  VkSurfaceKHR m_surface;                      ///< surface
  std::unique_ptr<vkb::Swapchain> m_swapchain; ///< swapchain
  bool m_invalidSwapchain = false;
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  std::vector<VkSemaphore> m_imageAvailabilitySemaphores;
  uint32_t m_activeSemaphore = 0;

private:
  void DestroySwapchain() noexcept;
};

} // namespace RHI::vulkan
