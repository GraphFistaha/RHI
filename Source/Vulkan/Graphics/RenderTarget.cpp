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

void RenderTarget::SetClearColor(float r, float g, float b, float a) noexcept
{
  m_clearValue = VkClearValue{r, g, b, a};
}

std::pair<uint32_t, uint32_t> RenderTarget::GetExtent() const noexcept
{
  return {m_extent.width, m_extent.height};
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

void RenderTarget::BindAttachment(uint32_t index, const FramebufferAttachment & attachment)
{
  while (index >= m_attachments.size())
    m_attachments.emplace_back();
  m_attachments[index] = attachment;
  m_builder.BindAttachment(index, attachment.GetImageView());
  m_invalidFramebuffer = true;
}

VkFramebuffer RenderTarget::GetHandle() const noexcept
{
  return m_framebuffer;
}

void RenderTarget::SetExtent(const VkExtent2D & extent) noexcept
{
  if (m_extent != extent)
  {
    m_extent = extent;
    m_invalidFramebuffer = true;
  }
}


} // namespace RHI::vulkan
