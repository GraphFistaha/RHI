#include "RenderTarget.hpp"

#include <algorithm>

#include <VkBootstrap.h>

#include "../CommandsExecution/CommandBuffer.hpp"
#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{

RenderTarget::RenderTarget(Context & ctx)
  : OwnedBy<Context>(ctx)
{
}

RenderTarget::~RenderTarget()
{
  GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_framebuffer, nullptr);
}

RenderTarget::RenderTarget(RenderTarget && rhs) noexcept
  : OwnedBy<Context>(std::move(rhs))
{
  std::swap(m_boundRenderPass, rhs.m_boundRenderPass);
  std::swap(m_attachedImages, rhs.m_attachedImages);
  std::swap(m_extent, rhs.m_extent);
  std::swap(m_clearValues, rhs.m_clearValues);
  std::swap(m_framebuffer, rhs.m_framebuffer);
  std::swap(m_builder, rhs.m_builder);
  std::swap(m_invalidFramebuffer, rhs.m_invalidFramebuffer);
}

RenderTarget & RenderTarget::operator=(RenderTarget && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_boundRenderPass, rhs.m_boundRenderPass);
    std::swap(m_attachedImages, rhs.m_attachedImages);
    std::swap(m_extent, rhs.m_extent);
    std::swap(m_clearValues, rhs.m_clearValues);
    std::swap(m_framebuffer, rhs.m_framebuffer);
    std::swap(m_builder, rhs.m_builder);
    std::swap(m_invalidFramebuffer, rhs.m_invalidFramebuffer);
  }
  return *this;
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

ImageExtent RenderTarget::GetExtent() const noexcept
{
  return {m_extent.width, m_extent.height, m_extent.depth};
}

void RenderTarget::Invalidate()
{
  assert(m_boundRenderPass);
  if (m_invalidFramebuffer || !m_framebuffer)
  {
    m_builder.Reset();
    for (uint32_t i = 0; i < m_attachedImages.size(); ++i)
      m_builder.BindAttachment(i, m_attachedImages[i].GetHandle());
    // TODO: init m_extent

    auto new_framebuffer = m_builder.Make(GetContext().GetDevice(), m_boundRenderPass, m_extent);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "build new VkFramebuffer");
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_framebuffer, nullptr);
    m_framebuffer = new_framebuffer;
    m_invalidFramebuffer = false;
  }
}

void RenderTarget::BindRenderPass(const VkRenderPass & renderPass) noexcept
{
  if (renderPass != m_boundRenderPass)
  {
    m_boundRenderPass = renderPass;
    m_invalidFramebuffer = true;
  }
}

const std::vector<VkClearValue> & RenderTarget::GetClearValues() const & noexcept
{
  return m_clearValues;
}

void RenderTarget::SetAttachments(std::vector<ImageView> && views) noexcept
{
  m_attachedImages = std::move(views);
  m_invalidFramebuffer = true;
}

void RenderTarget::AddAttachment(uint32_t index, ImageView && view)
{
  while (m_attachedImages.size() < index + 1)
    m_attachedImages.emplace_back(GetContext());
  m_attachedImages[index] = std::move(view);
  m_invalidFramebuffer = true;
}

void RenderTarget::ForEachAttachedImage(ProcessImagesFunc && func) noexcept
{
  for (auto && view : m_attachedImages)
    func(*view.GetImagePtr());
}

size_t RenderTarget::GetAttachmentsCount() const noexcept
{
  return m_attachedImages.size();
}

void RHI::vulkan::RenderTarget::ClearAttachments() noexcept
{
  m_attachedImages.clear();
  m_clearValues.clear();
  m_invalidFramebuffer = true;
}


} // namespace RHI::vulkan
