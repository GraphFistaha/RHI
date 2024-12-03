#pragma once

#include <deque>
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "RenderTarget.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct SwapchainBase : public ISwapchain
{
  explicit SwapchainBase(const Context & ctx);
  virtual ~SwapchainBase() override;

public: // ISwapchain interface
  virtual std::pair<uint32_t, uint32_t> GetExtent() const override;
  virtual IRenderTarget * AcquireFrame() override;
  virtual void FlushFrame() override;
  virtual ISubpass * CreateSubpass() override;

protected: // SwapchainBase interface
  virtual uint32_t AcquireImage(VkSemaphore signalSemaphore) noexcept = 0;
  virtual bool PresentImage(uint32_t activeImage, VkSemaphore waitRenderingSemaphore) noexcept = 0;

public: // RHI-only API
  virtual void DestroyHandles() noexcept;
  size_t GetImagesCount() const noexcept;

protected:
  void InvalidateRenderTargets(const std::vector<FramebufferAttachment> & attachments) noexcept; 

protected:
  static constexpr uint32_t InvalidImageIndex = -1;
  static constexpr uint32_t MaxFramesInFlight = 2;
  const Context & m_context;

  VkExtent2D m_extent{0, 0};
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  std::vector<VkSemaphore> m_imageAvailabilitySemaphores;
  uint32_t m_activeImage = InvalidImageIndex;
  uint32_t m_activeSemaphore = 0;

  std::deque<RenderTarget> m_targets;
  std::unique_ptr<RenderPass> m_renderPass;
};

} // namespace RHI::vulkan
