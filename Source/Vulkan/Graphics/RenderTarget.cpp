#include "RenderTarget.hpp"

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
  std::swap(m_views, rhs.m_views);
  std::swap(m_extent, rhs.m_extent);
  std::swap(m_clearValues, rhs.m_clearValues);
  std::swap(m_images, rhs.m_images);
  std::swap(m_framebuffer, rhs.m_framebuffer);
  std::swap(m_builder, rhs.m_builder);
  // why std::swap for bool doesn't work???
  bool tmp = m_invalidFramebuffer;
  m_invalidFramebuffer = rhs.m_invalidFramebuffer;
  rhs.m_invalidFramebuffer = tmp;
}

RenderTarget & RenderTarget::operator=(RenderTarget && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_boundRenderPass, rhs.m_boundRenderPass);
    std::swap(m_views, rhs.m_views);
    std::swap(m_extent, rhs.m_extent);
    std::swap(m_clearValues, rhs.m_clearValues);
    std::swap(m_images, rhs.m_images);
    std::swap(m_framebuffer, rhs.m_framebuffer);
    std::swap(m_builder, rhs.m_builder);
    // why std::swap for bool doesn't work???
    bool tmp = m_invalidFramebuffer;
    m_invalidFramebuffer = rhs.m_invalidFramebuffer;
    rhs.m_invalidFramebuffer = tmp;
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

IImageGPU * RenderTarget::GetImage(uint32_t attachmentIndex) const
{
    //TODO: rewrite
    return nullptr;//m_images[attachmentIndex].get();
}

void RenderTarget::Invalidate()
{
  assert(m_boundRenderPass);
  if (m_invalidFramebuffer || !m_framebuffer)
  {
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

void RenderTarget::AddAttachment(uint32_t index, std::unique_ptr<Image> && image,
                                 ImageView && view)
{
  while (index >= m_images.size())
  {
    m_views.emplace_back(GetContext());
    m_clearValues.emplace_back();
    m_images.emplace_back(nullptr);
  }
  m_builder.BindAttachment(index, view.GetHandle());
  m_invalidFramebuffer = true;

  m_views[index] = std::move(view);
  m_images[index] = std::move(image);
}

void RenderTarget::ForEachAttachedImage(ProcessImagesFunc && func) noexcept
{
  for (auto && imagePtr : m_images)
    func(*imagePtr);
}

size_t RenderTarget::GetAttachmentsCount() const noexcept
{
  return m_images.size();
}

void RHI::vulkan::RenderTarget::ClearAttachments() noexcept
{
  m_images.clear();
  m_views.clear();
  m_clearValues.clear();
  m_builder.Reset();
  m_invalidFramebuffer = true;
}

void RenderTarget::SetExtent(const VkExtent3D & extent) noexcept
{
  if (vk::Extent3D(m_extent) != vk::Extent3D(extent))
  {
    m_extent = extent;
    m_invalidFramebuffer = true;
  }
}


} // namespace RHI::vulkan
