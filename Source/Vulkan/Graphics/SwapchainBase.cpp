#include "SwapchainBase.hpp"

#include <format>

#include "../VulkanContext.hpp"
#include "RenderPass.hpp"

namespace RHI::vulkan
{

SwapchainBase::SwapchainBase(const Context & ctx)
  : m_context(ctx)
{
  m_renderPass = std::make_unique<RenderPass>(m_context);
}

SwapchainBase::~SwapchainBase()
{
  DestroyHandles();
}

void SwapchainBase::DestroyHandles() noexcept
{
  m_targets.clear();
  for (auto && semaphore : m_imageAvailabilitySemaphores)
  {
    if (!!semaphore)
      vkDestroySemaphore(m_context.GetDevice(), semaphore, nullptr);
  }
  m_imageAvailabilitySemaphores.clear();
}

size_t SwapchainBase::GetImagesCount() const noexcept
{
  return m_swapchainImages.size();
}

void SwapchainBase::InvalidateRenderTargets(
  const std::vector<FramebufferAttachment> & attachments) noexcept
{
  for (size_t i = 0, c = GetImagesCount(); i < c; ++i)
  {
    auto && target = m_targets.emplace_back(m_context);
    for (auto && attachment : attachments)
      target.AddAttachment(attachment);
    target.SetExtent(m_extent);
    target.BindRenderPass(m_renderPass->GetHandle());
  }
}

std::pair<uint32_t, uint32_t> SwapchainBase::GetExtent() const
{
  return {m_extent.width, m_extent.height};
}

IRenderTarget * SwapchainBase::AcquireFrame()
{
  uint32_t imageIndex = AcquireImage(m_imageAvailabilitySemaphores[m_activeSemaphore]);
  if (imageIndex == InvalidImageIndex)
  {
    return nullptr;
  }

  m_activeImage = imageIndex;
  auto && activeTarget = m_targets[m_activeImage];

  m_renderPass->BindRenderTarget(&activeTarget);
  m_renderPass->Invalidate();
  activeTarget.BindRenderPass(m_renderPass->GetHandle());
  activeTarget.Invalidate();
  return &activeTarget;
}

void SwapchainBase::FlushFrame()
{
  VkSemaphore passSemaphore = m_renderPass->Draw(m_imageAvailabilitySemaphores[m_activeSemaphore]);
  PresentImage(m_activeImage, passSemaphore);
  m_activeSemaphore = (m_activeSemaphore + 1) % m_imageAvailabilitySemaphores.size();
}

ISubpass * SwapchainBase::CreateSubpass()
{
  return m_renderPass->CreateSubpass();
}

} // namespace RHI::vulkan
