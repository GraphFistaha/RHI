#pragma once

#include <bitset>
#include <vector>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

/// @brief vulkan implementation for renderer
struct Swapchain : public ISwapchain
{
  explicit Swapchain(const Context & ctx);
  virtual ~Swapchain() override;

public: // ISwapchain interface
  /// begins rendering 
  virtual IRenderTarget * AcquireFrame() override;
  /// finish rendering
  virtual IAwaitable * RenderFrame() override;

  virtual ISubpass * CreateSubpass() override;
  /// @brief adds attachment to all frames
  /// @param binding - index of binding
  /// @param args - arguments for image creation
  virtual void AddImageAttachment(uint32_t binding, const ImageCreateArguments & description) override;
  /// @brief removes all images from all frames
  virtual void ClearImageAttachments() noexcept override;
  /// @brief operation which add or remove some frames from swapchain
  /// @param frames_count
  virtual void SetFramesCount(uint32_t frames_count) noexcept override;
  /// @brief expensive operation, which will rebuild swapchain fully
  /// @param extent - size of renderable image
  virtual void SetExtent(const ImageExtent & extent) noexcept override;
  virtual void SetMultisampling(RHI::SamplesCount samples) noexcept override;

public: // RHI-only API
  size_t GetImagesCount() const noexcept;
  virtual void Invalidate();

protected:
  virtual std::pair<uint32_t, VkSemaphore> AcquireImage();
  virtual bool FinishImage(uint32_t activeImage, AsyncTask * task);
  virtual void InvalidateAttachments();
  void RequireSwapchainHasAttachmentsCount(uint32_t count);

protected:
  static constexpr uint32_t InvalidImageIndex = -1;

  const Context & m_context;
  std::vector<RenderTarget> m_targets;
  RenderPass m_renderPass;

  RHI::ImageExtent m_extent = {0, 0, 0};
  bool m_extentChanged = false;
  uint32_t m_framesCount = 0;
  bool m_framesCountChanged = false;
  RHI::SamplesCount m_samplesCount = RHI::SamplesCount::One;
  bool m_samplesCountChanged = false;
  std::vector<bool> m_ownedImages;
  std::vector<RHI::ImageCreateArguments> m_imageDescriptions;
  std::vector<VkAttachmentDescription> m_attachmentDescriptions;
  bool m_attachmentsChanged = false;

  std::atomic_bool m_frameStarted = false;
  uint32_t m_activeImageIdx = InvalidImageIndex;
  VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
};

} // namespace RHI::vulkan


/// @brief Compare operator for VkAttachmentDescription
inline bool operator==(const VkAttachmentDescription & lhs,
                       const VkAttachmentDescription & rhs) noexcept
{
  return std::memcmp(&lhs, &rhs, sizeof(VkAttachmentDescription)) == 0;
}
