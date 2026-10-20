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

struct RenderTargetInfo final
{
  /// ImageViews to render to
  std::vector<VkImageView> renderingImages;
  /// wait for images are ready for rendering
  std::vector<VkSemaphore> semaphores;
  /// clear values for each attachment
  std::vector<VkClearValue> clearValues;

  bool operator==(const RenderTargetInfo & rhs) const noexcept;
  operator bool() const noexcept;
};

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

  void SetAttachments(RenderTargetInfo && info) noexcept;
  void ClearAttachments() noexcept;
  size_t GetAttachmentsCount() const noexcept;

protected:
  VkRenderPass m_boundRenderPass = VK_NULL_HANDLE;
  /// cached size of all image attachments. ALl sizes of all images must be equal
  VkExtent3D m_extent{0, 0, 0};
  ///data for render target
  RenderTargetInfo m_info;

  VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
  bool m_invalidFramebuffer = false;
};

} // namespace RHI::vulkan
