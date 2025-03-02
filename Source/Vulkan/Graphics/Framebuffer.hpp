#pragma once

#include <bitset>
#include <vector>

#include <OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Images/SwapchainImage.hpp"
#include "RenderPass.hpp"
#include "RenderTarget.hpp"

namespace RHI::vulkan
{

enum class ImageUsage
{
  Transfer = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
  FramebufferAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
  Sampler = VK_IMAGE_USAGE_SAMPLED_BIT
};

struct IAttachment
{
  virtual ~IAttachment() = default;
  virtual std::pair<uint32_t, VkSemaphore> AcquireNextImage() = 0;
  virtual const ImageView & GetImage(uint32_t frameIndex, ImageUsage imageUsage) const & = 0;
  virtual bool FinishImage(VkSemaphore waitSemaphore) = 0; // not sure. Only presentation requires
  virtual void SetFramesCount(uint32_t framesCount) = 0;
  virtual VkAttachmentDescription BuildDescription() const noexcept = 0;
};


/// @brief vulkan implementation for renderer
struct Framebuffer : public IFramebuffer,
                     public OwnedBy<Context>
{
  explicit Framebuffer(Context & ctx);
  virtual ~Framebuffer() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public: // IFramebuffer interface
  /// begins rendering
  virtual IRenderTarget * BeginFrame() override;
  /// finish rendering
  virtual IAwaitable * EndFrame() override;
  ///
  virtual ISubpass * CreateSubpass() override;
  /// @brief adds attachment to all frames
  /// @param binding - index of binding
  /// @param args - arguments for image creation
  virtual void AddImageAttachment(uint32_t binding, std::shared_ptr<IImageGPU> image) override;
  /// @brief removes all images from all frames
  virtual void ClearImageAttachments() noexcept override;
  /// @brief operation which add or remove some frames from swapchain
  /// @param frames_count
  virtual void SetFramesCount(uint32_t frames_count) override;

public: // RHI-only API
  size_t GetImagesCount() const noexcept;
  void Invalidate();

protected:
  std::vector<RenderTarget> m_targets;
  uint32_t m_activeTarget = 0;
  RenderPass m_renderPass;

  std::vector<std::shared_ptr<IAttachment>> m_attachments;
  bool m_attachmentsChanged = false;
  std::vector<VkAttachmentDescription> m_attachmentDescriptions;
  std::vector<VkSemaphore> m_imagesAvailabilitySemaphores;
  std::vector<uint32_t> m_selectedImageIndices;

  uint32_t m_framesCount = 0;

  std::atomic_bool m_frameStarted = false;
};

} // namespace RHI::vulkan


/// @brief Compare operator for VkAttachmentDescription
inline bool operator==(const VkAttachmentDescription & lhs,
                       const VkAttachmentDescription & rhs) noexcept
{
  return std::memcmp(&lhs, &rhs, sizeof(VkAttachmentDescription)) == 0;
}
