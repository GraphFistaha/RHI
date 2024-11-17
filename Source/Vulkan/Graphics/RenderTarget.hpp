#pragma once
#include <list>

#include "../VulkanContext.hpp"
#include "FramebufferAttachment.hpp"

namespace RHI::vulkan
{
struct RenderPass;

namespace details
{
struct FramebufferBuilder;
} // namespace details

struct RenderTarget : public IRenderTarget
{
  explicit RenderTarget(const Context & ctx);
  virtual ~RenderTarget() override;

public: // IRenderTarget interface
  virtual void SetClearColor(float r, float g, float b, float a) noexcept override;
  virtual std::pair<uint32_t, uint32_t> GetExtent() const noexcept override;

public: // IInvalidable interface
  virtual void Invalidate() override;

public:
  void SetExtent(const VkExtent2D & extent) noexcept;

  void BindRenderPass(const vk::RenderPass & renderPass) noexcept;

  vk::Framebuffer GetHandle() const noexcept;
  VkClearValue GetClearValue() const noexcept { return m_clearValue; }
  VkExtent2D GetVkExtent() const noexcept { return m_extent; }

  const std::vector<FramebufferAttachment> & GetAttachments() const & noexcept;
  void AddAttachment(const FramebufferAttachment & attachment);

protected:
  const Context & m_context;
  vk::RenderPass m_boundRenderPass = nullptr;
  std::vector<FramebufferAttachment> m_attachments;

protected: // framebuffer params
  vk::Extent2D m_extent;
  VkClearValue m_clearValue; // make it array (one for each attachment)

protected: // handle
  vk::Framebuffer m_framebuffer = VK_NULL_HANDLE;
  std::unique_ptr<details::FramebufferBuilder> m_builder;
  bool m_invalidFramebuffer : 1 = false;
};

} // namespace RHI::vulkan
