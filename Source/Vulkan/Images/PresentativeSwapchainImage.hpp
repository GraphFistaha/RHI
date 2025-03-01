#pragma once
#include <deque>
#include <vector>

#include "../CommandsExecution/AsyncTask.hpp"
#include "SwapchainImage.hpp"

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct PresentativeSwapchainImage final : public SwapchainImage
{
  explicit PresentativeSwapchainImage(Context & ctx, const VkSurfaceKHR surface);
  virtual ~PresentativeSwapchainImage() override;

public: // RHI-only API
  VkSwapchainKHR GetHandle() const noexcept;

  // attachment interface
  std::pair<uint32_t, VkSemaphore> AcquireImage();
  bool FinishImage(uint32_t activeImage, VkSemaphore waitSemaphore);

  /// @brief destroys old surface data like framebuffers, images, images_views, ets and creates new
  void Invalidate();

protected:
  void InvalidateAttachments();
  void DestroySwapchain() noexcept;

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
};

} // namespace RHI::vulkan
