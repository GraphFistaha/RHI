#pragma once
#include <list>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Images/ImageBase.hpp"
#include "../Images/ImageView.hpp"
#include "../Utils/FramebufferBuilder.hpp"

namespace RHI::vulkan
{
struct Context;
}

namespace RHI::vulkan
{

struct RenderTarget : public IRenderTarget
{
  explicit RenderTarget(const Context & ctx);
  virtual ~RenderTarget() override;
  RenderTarget(RenderTarget && rhs) noexcept;
  RenderTarget & operator=(RenderTarget && rhs) noexcept;

public: // IRenderTarget interface
  virtual void SetClearValue(uint32_t attachmentIndex, float r, float g, float b,
                             float a) noexcept override;
  virtual void SetClearValue(uint32_t attachmentIndex, float depth,
                             uint32_t stencil) noexcept override;
  virtual ImageExtent GetExtent() const noexcept override;
  virtual IImageGPU * GetImage(uint32_t attachmentIndex) const override;

public:
  void Invalidate();
  void SetExtent(const VkExtent3D & extent) noexcept;
  void BindRenderPass(const VkRenderPass & renderPass) noexcept;

  VkFramebuffer GetHandle() const noexcept { return m_framebuffer; }
  VkExtent3D GetVkExtent() const noexcept { return m_extent; }
  const std::vector<VkClearValue> & GetClearValues() const & noexcept;

  void AddAttachment(uint32_t index, std::unique_ptr<ImageBase> && image, ImageView && view);
  void ClearAttachments() noexcept;

protected:
  const Context & m_context;
  VkRenderPass m_boundRenderPass = VK_NULL_HANDLE;

  VkExtent3D m_extent;
  std::vector<std::unique_ptr<ImageBase>> m_images;
  std::vector<ImageView> m_views;
  std::vector<VkClearValue> m_clearValues;

protected: // handle
  VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
  utils::FramebufferBuilder m_builder;
  bool m_invalidFramebuffer : 1 = false;

private:
  RenderTarget(const RenderTarget &) = delete;
  RenderTarget & operator=(const RenderTarget &) = delete;
};

} // namespace RHI::vulkan
