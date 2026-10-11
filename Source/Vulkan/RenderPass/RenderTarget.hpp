#pragma once
#include <list>

#include <Private/Constants.hpp>
#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <Utils/FramebufferBuilder.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct RenderTarget final : public OwnedBy<Context>
{
  explicit RenderTarget(Context & ctx);
  virtual ~RenderTarget() override;
  RenderTarget(RenderTarget && rhs) noexcept;
  RenderTarget & operator=(RenderTarget && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  RESTRICTED_COPY(RenderTarget);

public:
  void RebuildFramebuffer();
  void BindRenderPass(const VkRenderPass & renderPass) noexcept;
  void SetExtent(const VkExtent3D & extent) noexcept;

  VkFramebuffer GetHandle() const noexcept { return m_framebuffer; }
  VkExtent3D GetVkExtent() const noexcept { return m_extent; }
  std::span<const VkClearValue> GetClearValues() const noexcept;
  std::span<const VkImageView> GetImageViews() const noexcept;
  std::span<const VkSemaphore> GetImageAvailableForRenderSemaphores() const noexcept;

  void SetAttachments(MultibufferVector<VkImageView> && views,
                      MultibufferVector<VkClearValue> && clearValues,
                      MultibufferVector<VkSemaphore> && semaphores) noexcept;
  void ClearAttachments() noexcept;
  size_t GetAttachmentsCount() const noexcept;

protected:
  VkRenderPass m_boundRenderPass = VK_NULL_HANDLE;
  /// cached size of all image attachments. ALl sizes of all images must be equal
  VkExtent3D m_extent;
  /// ImageViews
  MultibufferVector<VkImageView> m_attachedImages;
  /// clear values for each attachment
  MultibufferVector<VkClearValue> m_clearValues;
  /// wait for images are ready for rendering
  MultibufferVector<VkSemaphore> m_imageAvailabilitySemaphores;

  VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
  bool m_invalidFramebuffer = false;
};

} // namespace RHI::vulkan
