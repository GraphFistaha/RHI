#include "RenderTarget.hpp"

#include <VkBootstrap.h>

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{

RenderTarget::RenderTarget(const Context & ctx)
  : m_context(ctx)
{
}

RenderTarget::~RenderTarget()
{
  m_context.GetGarbageCollector().PushVkObjectToDestroy(m_framebuffer, nullptr);
}

void RenderTarget::SetClearValue(uint32_t attachmentIndex, float r, float g, float b,
                                 float a) noexcept
{
  m_clearValues[attachmentIndex].color = VkClearColorValue{r, g, b, a};
}

void RenderTarget::SetClearValue(uint32_t attachmentIndex, float depth, uint32_t stencil) noexcept
{
  m_clearValues[attachmentIndex].depthStencil = VkClearDepthStencilValue{depth, stencil};
}

std::pair<uint32_t, uint32_t> RenderTarget::GetExtent() const noexcept
{
  return {m_extent.width, m_extent.height};
}

IImageGPU * RenderTarget::GetImage(uint32_t attachmentIndex) const
{
  return m_images[attachmentIndex].get();
}

void RenderTarget::Invalidate()
{
  assert(m_boundRenderPass);
  if (m_invalidFramebuffer || !m_framebuffer)
  {
    auto new_framebuffer = m_builder.Make(m_context.GetDevice(), m_boundRenderPass, m_extent);
    m_context.Log(RHI::LogMessageStatus::LOG_DEBUG, "build new VkFramebuffer");
    m_context.GetGarbageCollector().PushVkObjectToDestroy(m_framebuffer, nullptr);
    m_framebuffer = new_framebuffer;
    m_invalidFramebuffer = false;
  }
}

void RenderTarget::SetInvalid()
{
  m_invalidFramebuffer = true;
}

void RenderTarget::BindRenderPass(const VkRenderPass & renderPass) noexcept
{
  if (renderPass != m_boundRenderPass)
  {
    m_boundRenderPass = renderPass;
    m_invalidFramebuffer = true;
  }
}

const std::vector<FramebufferAttachment> & RenderTarget::GetAttachments() const & noexcept
{
  return m_attachments;
}

void RenderTarget::AddAttachment(uint32_t index, const FramebufferAttachment & description,
                                 std::unique_ptr<IImageGPU> && image)
{
  while (index > m_attachments.size())
  {
    m_attachments.emplace_back();
    m_clearValues.emplace_back();
    m_images.emplace_back(nullptr);
  }
  m_attachments.emplace_back(description);
  m_clearValues.emplace_back();
  m_images.emplace_back(std::move(image));
  m_builder.BindAttachment(index, description.GetImageView());
  m_invalidFramebuffer = true;
}

void RHI::vulkan::RenderTarget::ClearAttachments() noexcept
{
  m_images.clear();
  m_attachments.clear();
  m_clearValues.clear();
  m_builder.Reset();
  m_invalidFramebuffer = true;
}

void RenderTarget::SetExtent(const VkExtent2D & extent) noexcept
{
  if (m_extent != extent)
  {
    m_extent = extent;
    // TODO: rebuild all images
    m_invalidFramebuffer = true;
  }
}


} // namespace RHI::vulkan
