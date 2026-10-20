#include "RenderTarget.hpp"

#include <algorithm>

#include <CommandsExecution/CommandBuffer.hpp>
#include <RenderPass/RenderPass.hpp>
#include <VkBootstrap.h>
#include <VulkanContext.hpp>

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
  std::swap(m_info, rhs.m_info);
  std::swap(m_extent, rhs.m_extent);
  std::swap(m_framebuffer, rhs.m_framebuffer);
  std::swap(m_invalidFramebuffer, rhs.m_invalidFramebuffer);
}

RenderTarget & RenderTarget::operator=(RenderTarget && rhs) noexcept
{
  if (this != &rhs)
  {
    OwnedBy<Context>::operator=(std::move(rhs));
    std::swap(m_boundRenderPass, rhs.m_boundRenderPass);
    std::swap(m_info, rhs.m_info);
    std::swap(m_extent, rhs.m_extent);
    std::swap(m_framebuffer, rhs.m_framebuffer);
    std::swap(m_invalidFramebuffer, rhs.m_invalidFramebuffer);
  }
  return *this;
}

void RenderTarget::RebuildFramebuffer()
{
  assert(m_boundRenderPass);
  if (m_invalidFramebuffer || !m_framebuffer)
  {
    if (m_extent.width == 0 || m_extent.height == 0 || m_extent.depth == 0)
      throw std::invalid_argument("Empty render target");
    utils::FramebufferBuilder builder;
    auto new_framebuffer = builder.Make(GetContext().GetGpuConnection().GetDevice(),
                                        m_boundRenderPass, m_extent, m_info.renderingImages);
    GetContext().GetGarbageCollector().PushVkObjectToDestroy(m_framebuffer, nullptr);
    GetContext().Log(RHI::LogMessageStatus::LOG_DEBUG, "VkFramebuffer({}) has been rebuilt - {}",
                     static_cast<void *>(m_framebuffer), static_cast<void *>(new_framebuffer));
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

void RenderTarget::SetExtent(const VkExtent3D & extent) noexcept
{
  m_extent = extent;
}

std::span<const VkClearValue> RenderTarget::GetClearValues() const noexcept
{
  return m_info.clearValues;
}

std::span<const VkImageView> RenderTarget::GetImageViews() const noexcept
{
  return m_info.renderingImages;
}

std::span<const VkSemaphore> RenderTarget::GetImageAvailableForRenderSemaphores() const noexcept
{
  return m_info.semaphores;
}

void RenderTarget::SetAttachments(RenderTargetInfo && info) noexcept
{
  if (m_info != info)
  {
    m_info = std::move(info);
    m_invalidFramebuffer = true;
  }
}

size_t RenderTarget::GetAttachmentsCount() const noexcept
{
  return m_info.renderingImages.size();
}

void RHI::vulkan::RenderTarget::ClearAttachments() noexcept
{
  m_info.renderingImages.clear();
  m_info.clearValues.clear();
  m_invalidFramebuffer = true;
}


bool RenderTargetInfo::operator==(const RenderTargetInfo & rhs) const noexcept
{
  return std::ranges::equal(renderingImages, rhs.renderingImages);
}

RenderTargetInfo::operator bool() const noexcept
{
  return !renderingImages.empty();
}

} // namespace RHI::vulkan
