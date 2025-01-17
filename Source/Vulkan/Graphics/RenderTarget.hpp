#pragma once
#include <list>

#include <RHI.hpp>
#include <vulkan/vulkan.hpp>

#include "../Images/ImageGPU.hpp"
#include "../Utils/FramebufferBuilder.hpp"
#include "FramebufferAttachment.hpp"
#include "RenderPass.hpp"

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

public: // IRenderTarget interface
  virtual void SetClearValue(uint32_t attachmentIndex, float r, float g, float b,
                             float a) noexcept override;
  virtual void SetClearValue(uint32_t attachmentIndex, float depth,
                             uint32_t stencil) noexcept override;
  virtual std::pair<uint32_t, uint32_t> GetExtent() const noexcept override;
  virtual IImageGPU * GetImage(uint32_t attachmentIndex) const override;

public: // IInvalidable interface
  virtual void Invalidate() override;
  virtual void SetInvalid() override;

public:
  void SetExtent(const VkExtent2D & extent) noexcept;
  void BindRenderPass(const VkRenderPass & renderPass) noexcept;

  VkFramebuffer GetHandle() const noexcept { return m_framebuffer; }
  VkExtent2D GetVkExtent() const noexcept { return m_extent; }

  const std::vector<FramebufferAttachment> & GetAttachments() const & noexcept;
  void AddAttachment(uint32_t index, const FramebufferAttachment & description,
                     std::unique_ptr<IImageGPU> && image);
  void ClearAttachments() noexcept;

protected:
  const Context & m_context;
  VkRenderPass m_boundRenderPass = nullptr;
  std::vector<FramebufferAttachment> m_attachments;

protected: // framebuffer params
  vk::Extent2D m_extent;
  std::vector<VkClearValue> m_clearValues;
  std::vector<std::unique_ptr<IImageGPU>> m_images;

protected: // handle
  VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
  utils::FramebufferBuilder m_builder;
  bool m_invalidFramebuffer : 1 = false;
};

} // namespace RHI::vulkan
