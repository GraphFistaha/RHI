#pragma once
#include <deque>
#include <vector>

#include "../VulkanContext.hpp"
#include "RenderTarget.hpp"

namespace vkb
{
struct Swapchain;
}

namespace RHI::vulkan
{
struct RenderPass;

/// @brief vulkan implementation for renderer
struct Swapchain final : public ISwapchain
{
  explicit Swapchain(const Context & ctx, const vk::SurfaceKHR surface);
  virtual ~Swapchain() override;

public: // ISwapchain interface
  virtual std::pair<uint32_t, uint32_t> GetExtent() const override;
  virtual IRenderTarget * AcquireFrame() override;
  virtual void FlushFrame() override;

  virtual ISubpass * CreateSubpass() override;

public: // IInvalidable interface
  /// @brief destroys old surface data like framebuffers, images, images_views, ets and creates new
  virtual void Invalidate() override;

public: // RHI-only API
  size_t GetImagesCount() const noexcept;
  vk::SwapchainKHR GetHandle() const noexcept;

private:
  static constexpr uint32_t InvalidImageIndex = -1;
  static constexpr uint32_t MaxFramesInFlight = 2;
  const Context & m_context;

  vk::Queue m_presentQueue = VK_NULL_HANDLE;
  uint32_t m_presentQueueIndex;

  /// presentation data
  vk::SurfaceKHR m_surface;       ///< surface
  bool m_usePresentation = false; ///< flag of presentation usage, true if surface is valid
  std::unique_ptr<vkb::Swapchain> m_swapchain; ///< swapchain

  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  std::vector<vk::Semaphore> m_imageAvailabilitySemaphores;
  std::deque<RenderTarget> m_targets;
  uint32_t m_activeImage = InvalidImageIndex;
  uint32_t m_activeSemaphore = 0;

  std::unique_ptr<RenderPass> m_renderPass;

private:
  /// destroy and create new swapchain
  void InvalidateSwapchain();
  void DestroyHandles() noexcept;
};

} // namespace RHI::vulkan
