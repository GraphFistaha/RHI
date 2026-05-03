#pragma once
#include <list>

#include <Private/OwnedBy.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Utils/FramebufferBuilder.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct RenderTarget : public OwnedBy<Context>
{
  explicit RenderTarget(Context & ctx);
  virtual ~RenderTarget() override;
  RenderTarget(RenderTarget && rhs) noexcept;
  RenderTarget & operator=(RenderTarget && rhs) noexcept;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  RESTRICTED_COPY(RenderTarget);

public:
  void Invalidate();
  void BindRenderPass(const VkRenderPass & renderPass) noexcept;
  void SetExtent(const VkExtent3D & extent) noexcept;

  VkFramebuffer GetHandle() const noexcept { return m_framebuffer; }
  VkExtent3D GetVkExtent() const noexcept { return m_extent; }
  const std::vector<VkClearValue> & GetClearValues() const & noexcept;

  void SetAttachments(std::vector<VkImageView> && views,
                      std::vector<VkClearValue> && clearValues) noexcept;
  void ClearAttachments() noexcept;
  size_t GetAttachmentsCount() const noexcept;

protected:
  VkRenderPass m_boundRenderPass = VK_NULL_HANDLE;
  /// cached size of all image attachments. ALl sizes of all images must be equal
  VkExtent3D m_extent;
  /// ImageViews
  std::vector<VkImageView> m_attachedImages;
  /// clear values for each attachment
  std::vector<VkClearValue> m_clearValues;

  VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
  utils::FramebufferBuilder m_builder;
  bool m_invalidFramebuffer = false;
};

} // namespace RHI::vulkan
